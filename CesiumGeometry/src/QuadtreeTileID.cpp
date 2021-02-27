// Copyright CesiumGS, Inc. and Contributors

#include "CesiumGeometry/QuadtreeTilingScheme.h"

namespace CesiumGeometry {

    uint32_t QuadtreeTileID::computeInvertedY(const QuadtreeTilingScheme& tilingScheme) const noexcept {
        uint32_t yTiles = tilingScheme.getNumberOfYTilesAtLevel(this->level);
        return yTiles - this->y - 1;
    }

}
