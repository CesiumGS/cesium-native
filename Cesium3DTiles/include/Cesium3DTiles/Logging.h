#pragma once

#include <memory>
#include <cstdarg>

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/ILogger.h"

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