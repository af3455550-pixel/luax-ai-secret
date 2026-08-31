#pragma once

#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include "../Threading/TaskGraph.hpp"

namespace Apex::Events {

    class IEvent {
    public:
        virtual ~IEvent() = default;
        virtual const char* GetName() const = 0;
    };

    class EventBus {
    public:
        using EventCallback = std::function<void(const IEvent&)>;

        static EventBus& Get() {
            static EventBus instance;
            return instance;
        }

        template <typename TEvent, typename TClass>
        void Subscribe(TClass* instance, void (TClass::*memberFn)(const TEvent&)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::type_index typeId = std::type_index(typeid(TEvent));

            m_subscribers[typeId].push_back([instance, memberFn](const IEvent& evt) {
                (instance->*memberFn)(static_cast<const TEvent&>(evt));
            });
        }

        template <typename TEvent>
        void Subscribe(std::function<void(const TEvent&)> callback) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::type_index typeId = std::type_index(typeid(TEvent));

            m_subscribers[typeId].push_back([callback](const IEvent& evt) {
                callback(static_cast<const TEvent&>(evt));
            });
        }

        template <typename TEvent, typename... Args>
        void PublishSync(Args&&... args) {
            TEvent evt(std::forward<Args>(args)...);
            std::type_index typeId = std::type_index(typeid(TEvent));

            std::vector<EventCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_subscribers.find(typeId);
                if (it != m_subscribers.end()) {
                    callbacks = it->second;
                }
            }

            for (auto& cb : callbacks) {
                cb(evt);
            }
        }

        template <typename TEvent, typename... Args>
        void PublishAsync(Args&&... args) {
            auto evtShared = std::make_shared<TEvent>(std::forward<Args>(args)...);
            Threading::TaskGraph::Get().Enqueue([this, evtShared]() {
                std::type_index typeId = std::type_index(typeid(TEvent));
                std::vector<EventCallback> callbacks;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    auto it = m_subscribers.find(typeId);
                    if (it != m_subscribers.end()) {
                        callbacks = it->second;
                    }
                }
                for (auto& cb : callbacks) {
                    cb(*evtShared);
                }
            });
        }

    private:
        std::unordered_map<std::type_index, std::vector<EventCallback>> m_subscribers;
        std::mutex m_mutex;
    };

} // namespace Apex::Events
