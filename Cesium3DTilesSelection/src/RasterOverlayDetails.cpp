#include <Cesium3DTilesSelection/RasterOverlayDetails.h>

#include <algorithm>

namespace Cesium3DTilesSelection {
const CesiumGeometry::Rectangle*
RasterOverlayDetails::findRectangleForOverlayProjection(
    const CesiumGeospatial::Projection& projection) const {
  const std::vector<CesiumGeospatial::Projection>& projections =
      this->rasterOverlayProjections;
  const std::vector<CesiumGeometry::Rectangle>& rectangles =
      this->rasterOverlayRectangles;

  assert(projections.size() == rectangles.size());

  auto it = std::find(projections.begin(), projections.end(), projection);
  if (it != projections.end()) {
    int32_t index = int32_t(it - projections.begin());
    if (index >= 0 && size_t(index) < rectangles.size()) {
      return &rectangles[size_t(index)];
    }
  }

  return nullptr;
}
} // namespace Cesium3DTilesSelection
