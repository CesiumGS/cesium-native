#include "CesiumGeometry/QuadtreeTilingScheme.h"

namespace CesiumGeometry {

uint32_t QuadtreeTileID::computeInvertedY(
    const QuadtreeTilingScheme& tilingScheme) const noexcept {
  const uint32_t yTiles = tilingScheme.getNumberOfYTilesAtLevel(this->level);
  return yTiles - this->y - 1;
}

} // namespace CesiumGeometry
