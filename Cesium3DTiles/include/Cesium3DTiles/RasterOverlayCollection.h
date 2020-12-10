#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include <vector>
#include <memory>
#include <gsl/span>

namespace Cesium3DTiles {

    class Tileset;

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
    class CESIUM3DTILES_API RasterOverlayCollection final {
    public:

        /** 
         * @brief Creates a new instance. 
         * 
         * @param tileset The tileset to which this instance belongs
         */
        RasterOverlayCollection(Tileset& tileset) noexcept;

        ~RasterOverlayCollection();

        /**
         * @brief Adds the given {@link RasterOverlay} to this collection.
         *
         * @param pOverlay The pointer to the overlay. This may not be `nullptr`.
         */
        void add(std::unique_ptr<RasterOverlay>&& pOverlay);

        /**
         * @brief Remove the given {@link RasterOverlay} from this collection.
         */
        void remove(RasterOverlay* pOverlay) noexcept;

        /**
         * @brief A constant iterator for {@link RasterOverlay} instances.
         */
        typedef std::vector<std::unique_ptr<RasterOverlay>>::const_iterator const_iterator;

        /**
         * @brief Returns an iterator at the beginning of this collection.
         */
        const_iterator begin() const noexcept { return this->_overlays.begin(); }

        /**
         * @brief Returns an iterator at the end of this collection.
         */
        const_iterator end() const noexcept { return this->_overlays.end(); }

    private:
        Tileset* _pTileset;
        std::vector<std::unique_ptr<RasterOverlay>> _overlays;
    };

}
