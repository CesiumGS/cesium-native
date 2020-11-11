#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include <vector>
#include <memory>
#include <gsl/span>

namespace Cesium3DTiles {

    class TilesetExternals;

    /**
     * @brief A collection of {@link RasterOverlay} instances that are associated with a {@link Tileset}.
     * 
     * The raster overlay instances may be added to the raster overlay collection
     * of a tileset that is returned with {@link Tileset::getOverlays}. When the
     * tileset is loaded, one {@link RasterOverlayTileProvider} will be created
     * for each raster overlay that had been added. The raster overlay tile provider
     * instances will be passed to the {@link RasterOverlayTile} instances that
     * they create when the tiles are updated.
     */
    class CESIUM3DTILES_API RasterOverlayCollection {
    public:

        /** @brief Creates a new instance. */
        RasterOverlayCollection();

        /** 
         * @brief Adds the given {@link RasterOverlay} to this collection.
         * 
         * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
         */
        void push_back(std::unique_ptr<RasterOverlay>&& pOverlay);

        /**
         * @brief Create the {@link RasterOverlayTileProvider} instances for all raster overlays.
         * 
         * This is called by the {@link Tileset}, after the tileset JSON
         * has been loaded. It will create one raster overlay tile provider
         * for each raster overlay that had previously been added.
         * 
         * @param assetAccessor The {@link TilesetExternals} that are associated
         * with the calling tileset.
         */
        void createTileProviders(TilesetExternals& assetAccessor);

        /**
         * @brief Returns a view on the {@link RasterOverlayTileProvider} instances of this collection.
         * 
         * @return The raster overlay tile provider instances.
         */
        gsl::span<RasterOverlayTileProvider*> getTileProviders() { return this->_quickTileProviders; }

        /**
         * @brief Returns the  {@link RasterOverlayTileProvider} for the given placeholder.
         * 
         * @param pPlaceholder The placeholder.
         * @return The raster overlay tile provider, or `nullptr` if no matching instance is found.
         */
        RasterOverlayTileProvider* findProviderForPlaceholder(RasterOverlayTileProvider* pPlaceholder);

    private:
        void overlayCreated(std::unique_ptr<RasterOverlayTileProvider>&& pOverlay);

        std::vector<std::unique_ptr<RasterOverlay>> _overlays;
        std::vector<std::unique_ptr<RasterOverlayTileProvider>> _placeholders;
        std::vector<std::unique_ptr<RasterOverlayTileProvider>> _tileProviders;
        std::vector<RasterOverlayTileProvider*> _quickTileProviders;

    };

}
