#pragma once

#include "Cesium3DTiles/Library.h"

#include <functional>

namespace Cesium3DTiles {

    class IAssetAccessor;
    class RasterOverlayTileProvider;

    class RasterOverlay {
    public:
        virtual ~RasterOverlay() = 0 {}

        /**
         * @brief A callback that receives the tile provider when it asynchronously becomes ready.
         * 
         * @param pTileProvider The newly-created tile provider.
         */
        typedef void CreateTileProviderCallback(std::unique_ptr<RasterOverlayTileProvider> pTileProvider);

        /**
         * @brief Asynchronously creates a new tile provider for this overlay.
         * 
         * @param assetAccessor The object used to download any assets that are required to create the tile provider.
         * @param callback The callback that receives the new tile provider when it is ready.
         */
        virtual void createTileProvider(IAssetAccessor& assetAccessor, std::function<CreateTileProviderCallback> callback) = 0;
    };

}
