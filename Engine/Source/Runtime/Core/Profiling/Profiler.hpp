#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <vector>
#include <algorithm>

namespace Apex::Profiling {

    struct ProfileSample {
        std::string name;
        double durationMs{0.0};
        int callCount{0};
    };

    class Profiler {
    public:
        static Profiler& Get() {
            static Profiler instance;
            return instance;
        }

        void Record(const std::string& name, double durationMs) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& sample = m_samples[name];
            sample.name = name;
            sample.durationMs += durationMs;
            sample.callCount++;
        }

        void BeginFrame() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_samples.clear();
            m_frameStartTime = std::chrono::high_resolution_clock::now();
        }

        void EndFrame() {
            auto now = std::chrono::high_resolution_clock::now();
            m_lastFrameTimeMs = std::chrono::duration<double, std::milli>(now - m_frameStartTime).count();
            m_frameCount++;
        }

        double GetLastFrameTimeMs() const { return m_lastFrameTimeMs; }
        double GetFPS() const { return m_lastFrameTimeMs > 0.0 ? 1000.0 / m_lastFrameTimeMs : 0.0; }
        uint64_t GetFrameCount() const { return m_frameCount; }

        std::vector<ProfileSample> GetSamples() {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<ProfileSample> list;
            for (auto& pair : m_samples) {
                list.push_back(pair.second);
            }
            std::sort(list.begin(), list.end(), [](const ProfileSample& a, const ProfileSample& b) {
                return a.durationMs > b.durationMs;
            });
            return list;
        }

    private:
        std::unordered_map<std::string, ProfileSample> m_samples;
        std::chrono::high_resolution_clock::time_point m_frameStartTime;
        double m_lastFrameTimeMs{16.66};
        uint64_t m_frameCount{0};
        std::mutex m_mutex;
    };

    class ScopedProfileTimer {
    public:
        explicit ScopedProfileTimer(std::string name)
            : m_name(std::move(name)), m_start(std::chrono::high_resolution_clock::now()) {}

        ~ScopedProfileTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            double dur = std::chrono::duration<double, std::milli>(end - m_start).count();
            Profiler::Get().Record(m_name, dur);
        }

    private:
        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_start;
    };

} // namespace Apex::Profiling

#define APEX_PROFILE_SCOPE(name) Apex::Profiling::ScopedProfileTimer _prof_##__LINE__(name)
#define APEX_PROFILE_FUNCTION() APEX_PROFILE_SCOPE(__FUNCTION__)
