#include "calcQuadtreeMaxGeometricError.h"

namespace Cesium3DTilesPipeline {
double
calcQuadtreeMaxGeometricError(const CesiumGeospatial::Ellipsoid& ellipsoid) {
  static const double mapQuality = 0.25;
  static const uint32_t mapWidth = 65;
  return ellipsoid.getMaximumRadius() * mapQuality / mapWidth;
}
} // namespace Cesium3DTilesPipeline