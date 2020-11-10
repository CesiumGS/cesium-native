#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayCutoutCollection.h"
#include <memory>
#include <functional>

namespace Cesium3DTiles {

    class TilesetExternals;
    class RasterOverlayTileProvider;

    /**
     * @brief The base class for a quadtree-tiled raster image that can be draped over a {@link Tileset}.
     * 
     * Instances of this class can be added to the {@link RasterOverlayCollection} 
     * that is returned by {@link Tileset::getOverlays}. 
     * 
     * @see BingMapsRasterOverlay
     * @see IonRasterOverlay
     * @see TileMapServiceRasterOverlay
     */
    class RasterOverlay {
    public:
        RasterOverlay();
        virtual ~RasterOverlay();

        /**
         * @brief A callback that receives the {@link RasterOverlayTileProvider} when it asynchronously becomes ready.
         * 
         * @param pTileProvider The newly-created tile provider, or `nullptr` if the tile provider could not be created.
         */
        typedef void CreateTileProviderCallback(std::unique_ptr<RasterOverlayTileProvider> pTileProvider);

        /**
         * @brief Asynchronously creates a new {@link RasterOverlayTileProvider} for this overlay.
         * 
         * @param tilesetExternals The external interfaces to use.
         * @param callback The callback that receives the new tile provider when it is ready.
         */
        virtual void createTileProvider(TilesetExternals& tilesetExternals, std::function<CreateTileProviderCallback>&& callback) = 0;

        /**
         * @brief Get a collection containing the sections of this overlay and its associated tileset that are not rendered.
         */
        RasterOverlayCutoutCollection& getCutouts() { return this->_cutouts; }

        /** @copydoc getCutouts() */
        const RasterOverlayCutoutCollection& getCutouts() const { return this->_cutouts; }

    private:
        RasterOverlayCutoutCollection _cutouts;
    };

}
