#include "Cesium3DTilesSelection/BoundingVolume.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {

BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume) {
  struct Operation {
    const glm::dmat4x4& transform;

    BoundingVolume operator()(const OrientedBoundingBox& boundingBox) {
      const glm::dvec3 center =
          glm::dvec3(transform * glm::dvec4(boundingBox.getCenter(), 1.0));
      const glm::dmat3 halfAxes =
          glm::dmat3(transform) * boundingBox.getHalfAxes();
      return OrientedBoundingBox(center, halfAxes);
    }

    BoundingVolume operator()(const BoundingRegion& boundingRegion) noexcept {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume operator()(const BoundingSphere& boundingSphere) {
      const glm::dvec3 center =
          glm::dvec3(transform * glm::dvec4(boundingSphere.getCenter(), 1.0));

      const double uniformScale = glm::max(
          glm::max(
              glm::length(glm::dvec3(transform[0])),
              glm::length(glm::dvec3(transform[1]))),
          glm::length(glm::dvec3(transform[2])));

      return BoundingSphere(center, boundingSphere.getRadius() * uniformScale);
    }

    BoundingVolume operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume
    operator()(const S2CellBoundingVolume& s2CellBoundingVolume) noexcept {
      // S2 Cells are not transformed.
      return s2CellBoundingVolume;
    }
  };

  return std::visit(Operation{transform}, boundingVolume);
}

glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume) {
  struct Operation {
    glm::dvec3 operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return boundingBox.getCenter();
    }

    glm::dvec3 operator()(const BoundingRegion& boundingRegion) noexcept {
      return boundingRegion.getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const BoundingSphere& boundingSphere) noexcept {
      return boundingSphere.getCenter();
    }

    glm::dvec3 operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return boundingRegion.getBoundingRegion().getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return s2Cell.getCenter();
    }
  };

  return std::visit(Operation{}, boundingVolume);
}

} // namespace Cesium3DTilesSelection
