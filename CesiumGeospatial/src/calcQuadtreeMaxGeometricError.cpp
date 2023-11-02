#include <CesiumGeospatial/calcQuadtreeMaxGeometricError.h>

namespace CesiumGeospatial {
double calcQuadtreeMaxGeometricError(
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  static const double mapQuality = 0.25;
  static const uint32_t mapWidth = 65;
  return ellipsoid.getMaximumRadius() * mapQuality / mapWidth;
}
} // namespace CesiumGeospatial
