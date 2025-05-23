#pragma once

#include <memory>  // Required for std::shared_ptr
#include <string>  // Required for std::string

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // For console output with colors
#include <spdlog/sinks/basic_file_sink.h>

namespace nsudb
{
    struct Logger final
    {
        Logger(const Logger&)            = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&)            = delete;
        Logger& operator=(Logger&&) = delete;

        static void Init() noexcept
        {
            if (s_bIsInitialized) return;

            // Create sinks
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);                     // Console shows all messages from trace up
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$: %v");  // Pattern with color

            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/nsudb_app.log", true);  // Single log file
            file_sink->set_level(spdlog::level::trace);                 // File logs all messages from trace up
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l]: %v");  // Pattern without color codes

            // Create the main spdlog logger with both sinks
            // If using async, replace spdlog::logger with spdlog::async_logger
            m_SpdLogger = std::make_shared<spdlog::logger>("nsudb_main_logger", spdlog::sinks_init_list({console_sink, file_sink}));

            // Set the overall log level for this logger instance
            m_SpdLogger->set_level(spdlog::level::trace);  // Default to most verbose
            m_SpdLogger->flush_on(spdlog::level::err);     // Flush immediately on error messages

            // Register the logger globally with spdlog (optional, but useful)
            spdlog::register_logger(m_SpdLogger);

            // Set this as the default logger for spdlog's global functions (e.g., spdlog::info)
            // This is important if you ever use spdlog::info(), spdlog::error(), etc., directly.
            spdlog::set_default_logger(m_SpdLogger);

            s_bIsInitialized = true;
        }

        static void Shutdown() noexcept
        {
            if (!s_bIsInitialized) return;

            // Unregister and drop the logger
            spdlog::drop("nsudb_main_logger");

            // Shutdown spdlog's thread pool if async logging was enabled
            spdlog::shutdown();
            s_bIsInitialized = false;
            m_SpdLogger.reset();  // Release the shared_ptr
        }

        static auto& GetLogger() noexcept
        {
            assert(s_bIsInitialized);
            return m_SpdLogger;
        }

      private:
        Logger() noexcept  = default;
        ~Logger() noexcept = default;

        static inline std::atomic_bool s_bIsInitialized{false};
        static inline std::shared_ptr<spdlog::logger> m_SpdLogger{nullptr};
    };

#define LOG_ERROR(msg, ...) Logger::GetLogger()->error(msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) Logger::GetLogger()->warn(msg, ##__VA_ARGS__)
#define LOG_TRACE(msg, ...) Logger::GetLogger()->trace(msg, ##__VA_ARGS__)

}  // namespace nsudb
