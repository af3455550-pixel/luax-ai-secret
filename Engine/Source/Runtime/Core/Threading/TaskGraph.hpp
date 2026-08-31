#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include "../Log/Logger.hpp"

namespace Apex::Threading {

    class TaskGraph {
    public:
        using Task = std::function<void()>;

        static TaskGraph& Get() {
            static TaskGraph instance;
            return instance;
        }

        void Initialize(size_t threadCount = 0) {
            if (m_initialized) return;

            if (threadCount == 0) {
                threadCount = std::max(2u, std::thread::hardware_concurrency());
            }

            m_stop = false;
            m_workers.reserve(threadCount);

            for (size_t i = 0; i < threadCount; ++i) {
                m_workers.emplace_back([this, i]() {
                    WorkerLoop(i);
                });
            }

            m_initialized = true;
            LOG_INFO("TaskGraph", "Initialized with " << threadCount << " worker threads.");
        }

        void Shutdown() {
            if (!m_initialized) return;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_stop = true;
            }
            m_condition.notify_all();

            for (std::thread& worker : m_workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            m_workers.clear();
            m_initialized = false;
            LOG_INFO("TaskGraph", "Shutdown completed.");
        }

        template <typename F, typename... Args>
        auto Enqueue(F&& f, Args&&... args) 
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                if (m_stop) {
                    throw std::runtime_error("Enqueue on stopped TaskGraph");
                }
                m_tasks.emplace([task]() { (*task)(); });
                m_activeTasks++;
            }
            m_condition.notify_one();
            return res;
        }

        void ParallelFor(size_t count, const std::function<void(size_t index)>& func) {
            if (count == 0) return;
            std::atomic<size_t> completed{0};
            std::mutex waitMutex;
            std::condition_variable waitCond;

            for (size_t i = 0; i < count; ++i) {
                Enqueue([&func, i, &completed, count, &waitCond, &waitMutex]() {
                    func(i);
                    if (++completed == count) {
                        std::lock_guard<std::mutex> lk(waitMutex);
                        waitCond.notify_one();
                    }
                });
            }

            std::unique_lock<std::mutex> lk(waitMutex);
            waitCond.wait(lk, [&]() { return completed.load() == count; });
        }

        size_t GetWorkerCount() const { return m_workers.size(); }
        size_t GetPendingTasks() {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            return m_tasks.size();
        }

    private:
        TaskGraph() = default;
        ~TaskGraph() { Shutdown(); }

        void WorkerLoop(size_t workerId) {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);
                    m_condition.wait(lock, [this]() {
                        return m_stop || !m_tasks.empty();
                    });

                    if (m_stop && m_tasks.empty()) {
                        return;
                    }

                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }

                try {
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("TaskGraph", "Worker " << workerId << " caught exception: " << e.what());
                }

                m_activeTasks--;
            }
        }

        std::vector<std::thread> m_workers;
        std::queue<Task> m_tasks;
        std::mutex m_queueMutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_stop{false};
        std::atomic<bool> m_initialized{false};
        std::atomic<size_t> m_activeTasks{0};
    };

} // namespace Apex::Threading
