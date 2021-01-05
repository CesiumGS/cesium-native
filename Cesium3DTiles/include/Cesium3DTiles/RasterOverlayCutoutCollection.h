#pragma once

#include "Cesium3DTiles/Library.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <vector>
#include <gsl/span>

namespace Cesium3DTiles {

    class TilesetExternals;

    /**
     * @brief A collection of rectangular areas cut out (not shown) from a {@link Tileset} with a {@link RasterOverlay}.
     */
    class CESIUM3DTILES_API RasterOverlayCutoutCollection final {
    public:
        RasterOverlayCutoutCollection() noexcept;

        /**
         * @brief Add the given {@link CesiumGeospatial::GlobeRectangle} to this collection.
         */
        void push_back(const CesiumGeospatial::GlobeRectangle& cutoutRectangle);

        /**
         * @brief Returns the {@link CesiumGeospatial::GlobeRectangle} objects of this collection.
         */
        gsl::span<const CesiumGeospatial::GlobeRectangle> getCutouts() const noexcept { return this->_cutouts; }

    private:
        std::vector<CesiumGeospatial::GlobeRectangle> _cutouts;
    };

}
