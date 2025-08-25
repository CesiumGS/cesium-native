#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumUtility/Hash.h>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace CesiumGeometry {
uint32_t QuadtreeTileID::computeInvertedX(
    const QuadtreeTilingScheme& tilingScheme) const noexcept {
  const uint32_t xTiles = tilingScheme.getNumberOfXTilesAtLevel(this->level);
  return xTiles - this->x - 1;
}

uint32_t QuadtreeTileID::computeInvertedY(
    const QuadtreeTilingScheme& tilingScheme) const noexcept {
  const uint32_t yTiles = tilingScheme.getNumberOfYTilesAtLevel(this->level);
  return yTiles - this->y - 1;
}

} // namespace CesiumGeometry

namespace std {

size_t hash<CesiumGeometry::QuadtreeTileID>::operator()(
    const CesiumGeometry::QuadtreeTileID& key) const noexcept {
  std::hash<uint32_t> h;
  size_t result = h(key.level);
  result = CesiumUtility::Hash::combine(result, h(key.x));
  result = CesiumUtility::Hash::combine(result, h(key.y));
  return result;
}

} // namespace std
