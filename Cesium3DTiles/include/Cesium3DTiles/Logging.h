#pragma once

#include <memory>
#include <cstdarg>

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/ILogger.h"

#include "../src/CesiumSpdlog.h"

#define CESIUM_LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define CESIUM_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define CESIUM_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define CESIUM_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define CESIUM_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define CESIUM_LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

namespace Cesium3DTiles {
    namespace Logging {

        /**
         * @brief Initialize the underlying logging infrastructure.
         */
        CESIUM3DTILES_API void initializeLogging() noexcept;

        /**
         * @brief Register the given logger to receive log messages. 
         * 
         * @param logger The {@link ILogger}. May not be `nullptr`.
         */
        CESIUM3DTILES_API void registerLogger(std::shared_ptr<ILogger> logger) noexcept;

        /**
         * @brief Unregister the given logger to no longer receive log messages.
         * 
         * @param logger The {@link ILogger}. May not be `nullptr`.
         */
        CESIUM3DTILES_API void unregisterLogger(std::shared_ptr<ILogger> logger) noexcept;

    }

}