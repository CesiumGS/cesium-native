#pragma once

#include "Cesium3DTiles/Library.h"
#include <spdlog/fwd.h>
#include <memory>

namespace CesiumAsync {
    class IAssetAccessor;
    class ITaskProcessor;
}

namespace Cesium3DTiles {
    class IPrepareRendererResources;

    /**
     * @brief External interfaces used by a {@link Tileset}.
     * 
     * Not supposed to be used by clients.
     */
    class CESIUM3DTILES_API TilesetExternals final {
    public:
        TilesetExternals() noexcept;

        /**
         * @brief An external {@link IAssetAccessor}.
         */
        std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

        /**
         * @brief An external {@link IPrepareRendererResources}.
         */
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources;

        /**
         * @brief An external {@link ITaskProcessor}
         */
        std::shared_ptr<CesiumAsync::ITaskProcessor> pTaskProcessor;

        /**
         * @brief A spdlog logger that will receive log messages.
         * 
         * If not specified, defaults to `spdlog::default_logger()`.
         */
        std::shared_ptr<spdlog::logger> pLogger;
    };

}
