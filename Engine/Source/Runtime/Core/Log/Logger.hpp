#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace Apex::Log {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal
    };

    class Logger {
    public:
        static Logger& Get() {
            static Logger instance;
            return instance;
        }

        void LogMessage(LogLevel level, const std::string& category, const std::string& msg) {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::tm tm_buf;
#if defined(_WIN32)
            localtime_s(&tm_buf, &time_t_now);
#else
            localtime_r(&time_t_now, &tm_buf);
#endif

            const char* colorCode = "\033[0m";
            const char* levelStr = "INFO";

            switch (level) {
                case LogLevel::Trace: colorCode = "\033[90m"; levelStr = "TRACE"; break;
                case LogLevel::Debug: colorCode = "\033[36m"; levelStr = "DEBUG"; break;
                case LogLevel::Info:  colorCode = "\033[32m"; levelStr = "INFO "; break;
                case LogLevel::Warn:  colorCode = "\033[33m"; levelStr = "WARN "; break;
                case LogLevel::Error: colorCode = "\033[31m"; levelStr = "ERROR"; break;
                case LogLevel::Fatal: colorCode = "\033[35;1m"; levelStr = "FATAL"; break;
            }

            std::cout << colorCode
                      << "[" << std::put_time(&tm_buf, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                      << "[" << levelStr << "] [" << category << "] "
                      << msg << "\033[0m\n";
        }

    private:
        std::mutex m_mutex;
    };

} // namespace Apex::Log

#define APEX_LOG(level, cat, ...) do { \
    std::ostringstream _ss; \
    _ss << __VA_ARGS__; \
    Apex::Log::Logger::Get().LogMessage(level, cat, _ss.str()); \
} while(0)

#define LOG_TRACE(cat, ...) APEX_LOG(Apex::Log::LogLevel::Trace, cat, __VA_ARGS__)
#define LOG_DEBUG(cat, ...) APEX_LOG(Apex::Log::LogLevel::Debug, cat, __VA_ARGS__)
#define LOG_INFO(cat, ...)  APEX_LOG(Apex::Log::LogLevel::Info, cat, __VA_ARGS__)
#define LOG_WARN(cat, ...)  APEX_LOG(Apex::Log::LogLevel::Warn, cat, __VA_ARGS__)
#define LOG_ERROR(cat, ...) APEX_LOG(Apex::Log::LogLevel::Error, cat, __VA_ARGS__)
#define LOG_FATAL(cat, ...) APEX_LOG(Apex::Log::LogLevel::Fatal, cat, __VA_ARGS__)
