#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayCutoutCollection.h"
#include <memory>
#include <functional>

namespace Cesium3DTiles {

    class TilesetExternals;
    class RasterOverlayTileProvider;
    class RasterOverlayCollection;

    /**
     * @brief The base class for a quadtree-tiled raster image that can be draped over a {@link Tileset}.
     */
    class RasterOverlay {
    public:
        RasterOverlay();
        virtual ~RasterOverlay();

        /**
         * @brief Gets the tile provider for this overlay.
         * 
         * Returns `nullptr` if {@link createTileProvider} has not yet been called.
         * If {@link createTileProvider} has been called but the overlay is not yet ready to
         * provide tiles, a placeholder tile provider will be returned.
         */
        RasterOverlayTileProvider* getTileProvider();
        const RasterOverlayTileProvider* getTileProvider() const;

        /**
         * @brief Get a collection containing the sections of this overlay and its associated tileset that are not rendered.
         */
        const RasterOverlayCutoutCollection& getCutouts() const { return this->_cutouts; }
        RasterOverlayCutoutCollection& getCutouts() { return this->_cutouts; }

        /**
         * @brief A callback that receives the tile provider when it asynchronously becomes ready.
         * 
         * @param pTileProvider The newly-created tile provider.
         */
        typedef void CreateTileProviderCallback(std::unique_ptr<RasterOverlayTileProvider>&& pTileProvider);

        /**
         * @brief Begins asynchronous creation of the tile provider for this overlay and eventually makes it available directly from this instance.
         * 
         * When the tile provider is ready, it will be returned by {@link getTileProvider}.
         * 
         * This method does nothing if the tile provider has already been created or
         * is already in the process of being created.
         * 
         * @param externals The external interfaces to use.
         */
        void createTileProvider(const TilesetExternals& externals);

        /**
         * @brief Begins asynchronous creation of the tile provider for this overlay and eventually returns it via a callback.
         * 
         * When the tile provider is ready, the `callback` will be invoked. The tile provider will not be returned
         * via {@link getTileProvider}. This method is primarily useful for overlays that aggregate other
         * overlays.
         * 
         * @param externals The external interfaces to use.
         * @param pOwner The overlay that owns this overlay, or nullptr if this overlay is not aggregated.
         * @param callback The callback that receives the new tile provider when it is ready.
         */
        virtual void createTileProvider(const TilesetExternals& externals, RasterOverlay* pOwner, std::function<CreateTileProviderCallback>&& callback) = 0;

    private:
        std::unique_ptr<RasterOverlayTileProvider> _pPlaceholder;
        std::unique_ptr<RasterOverlayTileProvider> _pTileProvider;
        RasterOverlayCutoutCollection _cutouts;
    };

}
