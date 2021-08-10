#include "TileUtilities.h"

#include <variant>

#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h"

namespace Cesium3DTilesSelection {
namespace Impl {

const CesiumGeospatial::GlobeRectangle* obtainGlobeRectangle(
    const Cesium3DTilesSelection::BoundingVolume* pBoundingVolume) noexcept {
  const CesiumGeospatial::BoundingRegion* pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(pBoundingVolume);
  if (pRegion) {
    return &pRegion->getRectangle();
  }
  const CesiumGeospatial::BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(
          pBoundingVolume);
  if (pLooseRegion) {
    return &pLooseRegion->getBoundingRegion().getRectangle();
  }
  return nullptr;
}
} // namespace Impl

} // namespace Cesium3DTilesSelection
