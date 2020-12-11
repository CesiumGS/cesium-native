
#include "Cesium3DTiles/Logging.h"
#include "Cesium3DTiles/ILogger.h"

#include "CesiumSpdlog.h"
#include "SpdlogLogger.h"

namespace Cesium3DTiles {
    namespace Logging {

        namespace {
            std::unordered_map<std::shared_ptr<ILogger>, spdlog::sink_ptr> loggerSinks;
        }
        
        void initializeLogging() noexcept {

            spdlog::set_level(spdlog::level::trace);

            // Set log message pattern for the default console sink of the default logger:
            // Hour, minutes, seconds, millseconds, thread ID, (color)level,8-left-aligned(/color), message
            spdlog::default_logger()->sinks()[0]->set_pattern("[%H:%M:%S:%e] [thread %-5t] [%^%-8l%$] : %v");

        }

        void registerLogger(std::shared_ptr<ILogger> logger) noexcept {
            if (logger == nullptr) {
                CESIUM_LOG_WARN("Cannot register nullptr as a logger");
                return;
            }
            auto it = loggerSinks.find(logger);
            if (it != loggerSinks.end()) {
                CESIUM_LOG_WARN("Logger is already registered");
                return;
            }
            auto loggerPtr = std::make_shared<spdlog_logger_sink_mt>(logger);
            auto sinkPtr = std::dynamic_pointer_cast<spdlog::sinks::sink>(loggerPtr);

            // Set the pattern for the sink:
            // Hour, minutes, seconds, millseconds, thread ID, message
            sinkPtr->set_pattern("[%H:%M:%S:%e] [thread %-5t] : %v");

            auto defaultLogger = spdlog::default_logger();
            auto& sinks = defaultLogger->sinks();
            sinks.push_back(sinkPtr);
            loggerSinks[logger] = sinkPtr;
        }

        void unregisterLogger(std::shared_ptr<ILogger> logger) noexcept {
            if (logger == nullptr) {
                CESIUM_LOG_WARN("Cannot unregister nullptr as a logger");
                return;
            }
            auto it = loggerSinks.find(logger);
            if (it == loggerSinks.end()) {
                CESIUM_LOG_WARN("Logger is not registered");
                return;
            }
            auto defaultLogger = spdlog::default_logger();
            auto& sinks = defaultLogger->sinks();
            sinks.pop_back();
            loggerSinks.erase(it);
        }

    }

}