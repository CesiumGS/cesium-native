#include <Cesium3DTilesSelection/RasterOverlayDetails.h>

#include <algorithm>
#include <limits>

namespace Cesium3DTilesSelection {
RasterOverlayDetails::RasterOverlayDetails()
    : boundingRegion{
          CesiumGeospatial::GlobeRectangle::EMPTY,
          std::numeric_limits<double>::max(),
          std::numeric_limits<double>::lowest()} {}

RasterOverlayDetails::RasterOverlayDetails(
    std::vector<CesiumGeospatial::Projection>&& rasterOverlayProjections_,
    std::vector<CesiumGeometry::Rectangle>&& rasterOverlayRectangles_,
    const CesiumGeospatial::BoundingRegion& boundingRegion_)
    : rasterOverlayProjections{std::move(rasterOverlayProjections_)},
      rasterOverlayRectangles{std::move(rasterOverlayRectangles_)},
      boundingRegion{boundingRegion_} {}

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

void RasterOverlayDetails::merge(const RasterOverlayDetails& other) {
  rasterOverlayProjections.insert(
      rasterOverlayProjections.end(),
      other.rasterOverlayProjections.begin(),
      other.rasterOverlayProjections.end());

  rasterOverlayRectangles.insert(
      rasterOverlayRectangles.end(),
      other.rasterOverlayRectangles.begin(),
      other.rasterOverlayRectangles.end());

  boundingRegion.computeUnion(other.boundingRegion);
}
} // namespace Cesium3DTilesSelection
