#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <glm/ext/vector_double3.hpp>

namespace CesiumGeospatial {

BoundingRegionWithLooseFittingHeights::BoundingRegionWithLooseFittingHeights(
    const BoundingRegion& boundingRegion) noexcept
    : _region(boundingRegion) {}

double BoundingRegionWithLooseFittingHeights::
    computeConservativeDistanceSquaredToPosition(
        const glm::dvec3& position,
        const Ellipsoid& ellipsoid) const noexcept {
  return this->_region.computeDistanceSquaredToPosition(position, ellipsoid);
}

double BoundingRegionWithLooseFittingHeights::
    computeConservativeDistanceSquaredToPosition(
        const Cartographic& position,
        const Ellipsoid& ellipsoid) const noexcept {
  return this->_region.computeDistanceSquaredToPosition(position, ellipsoid);
}

double BoundingRegionWithLooseFittingHeights::
    computeConservativeDistanceSquaredToPosition(
        const Cartographic& cartographicPosition,
        const glm::dvec3& cartesianPosition) const noexcept {
  return this->_region.computeDistanceSquaredToPosition(
      cartographicPosition,
      cartesianPosition);
}
} // namespace CesiumGeospatial
