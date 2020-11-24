
#include "spdlog/spdlog.h"

#include "Logging.h"

namespace Cesium3DTiles {
    namespace impl {
        void initializeLogging() noexcept {

            spdlog::set_level(spdlog::level::trace);

            // Set log message pattern:
            // Hour, minutes, seconds, millseconds, thread ID, (color)level,8-left-aligned(/color), message
            spdlog::set_pattern("[%H:%M:%S:%e] [thread %-5t] [%^%-8l%$] : %v");

        }
    }
}