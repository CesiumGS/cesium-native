
#include "spdlog/spdlog.h"

#include "Logging.h"

namespace Cesium3DTiles {
    namespace impl {
        void initializeLogging() noexcept {
            spdlog::set_level(spdlog::level::trace);
        }
    }
}