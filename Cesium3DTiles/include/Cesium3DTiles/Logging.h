#pragma once

#include <memory>
#include <cstdarg>

#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/spdlog.h"
#include "Cesium3DTiles/ILogger.h"

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

namespace Cesium3DTiles {
    namespace Logging {

        /**
         * @brief Initialize the underlying logging infrastructure.
         */
        void initializeLogging() noexcept;

        /**
         * @brief Register the given logger to receive log messages.
         * 
         * @param logger The {@link ILogger}. May not be `nullptr`.
         */
        void registerLogger(std::shared_ptr<ILogger> logger) noexcept;

        /**
         * @brief Unregister the given logger to no longer receive log messages.
         * 
         * @param logger The {@link ILogger}. May not be `nullptr`.
         */
        void unregisterLogger(std::shared_ptr<ILogger> logger) noexcept;

    }

}