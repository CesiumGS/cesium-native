#pragma once

#include "Cesium3DTiles/Library.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <memory>
#include <functional>

namespace Cesium3DTiles {

    class TilesetExternals;
    class RasterOverlayTileProvider;

    /**
     * @brief The base class for a quadtree-tiled raster image that can be draped over a {@link Tileset}.
     */
    class RasterOverlay {
    public:
        virtual ~RasterOverlay() {}

        /**
         * @brief A callback that receives the tile provider when it asynchronously becomes ready.
         * 
         * @param pTileProvider The newly-created tile provider.
         */
        typedef void CreateTileProviderCallback(std::unique_ptr<RasterOverlayTileProvider> pTileProvider);

        /**
         * @brief Asynchronously creates a new tile provider for this overlay.
         * 
         * @param tilesetExternals The external interfaces to use.
         * @param callback The callback that receives the new tile provider when it is ready.
         */
        virtual void createTileProvider(TilesetExternals& tilesetExternals, std::function<CreateTileProviderCallback>&& callback) = 0;

        std::vector<CesiumGeospatial::GlobeRectangle> cutouts;
    };

}
