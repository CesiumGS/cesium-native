#include "Cesium3DTilesSelection/TileContentLoadResult.h"

#include <algorithm>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;

const CesiumGeometry::Rectangle*
TileContentLoadResult::findRectangleForOverlayProjection(
    const CesiumGeospatial::Projection& projection) const {
  const std::vector<Projection>& projections = this->rasterOverlayProjections;
  const std::vector<Rectangle>& rectangles = this->rasterOverlayRectangles;

  assert(projections.size() == rectangles.size());

  auto it = std::find(projections.begin(), projections.end(), projection);
  if (it != projections.end()) {
    int32_t index = int32_t(it - projections.begin());
    if (index >= 0 && index < rectangles.size()) {
      return &rectangles[size_t(index)];
    }
  }

  return nullptr;
}
