#include "Cesium3DTiles/BoundingVolume.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume) {
  struct Operation {
    const glm::dmat4x4& transform;

    BoundingVolume operator()(const OrientedBoundingBox& boundingBox) {
      glm::dvec3 center = transform * glm::dvec4(boundingBox.getCenter(), 1.0);
      glm::dmat3 halfAxes = glm::dmat3(transform) * boundingBox.getHalfAxes();
      return OrientedBoundingBox(center, halfAxes);
    }

    BoundingVolume operator()(const BoundingRegion& boundingRegion) {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume operator()(const BoundingSphere& boundingSphere) {
      glm::dvec3 center =
          transform * glm::dvec4(boundingSphere.getCenter(), 1.0);

      double uniformScale = glm::max(
          glm::max(
              glm::length(glm::dvec3(transform[0])),
              glm::length(glm::dvec3(transform[1]))),
          glm::length(glm::dvec3(transform[2])));

      return BoundingSphere(center, boundingSphere.getRadius() * uniformScale);
    }

    BoundingVolume
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      // Regions are not transformed.
      return boundingRegion;
    }
  };

  return std::visit(Operation{transform}, boundingVolume);
}

glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume) {
  struct Operation {
    glm::dvec3 operator()(const OrientedBoundingBox& boundingBox) {
      return boundingBox.getCenter();
    }

    glm::dvec3 operator()(const BoundingRegion& boundingRegion) {
      return boundingRegion.getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const BoundingSphere& boundingSphere) {
      return boundingSphere.getCenter();
    }

    glm::dvec3
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      return boundingRegion.getBoundingRegion().getBoundingBox().getCenter();
    }
  };

  return std::visit(Operation{}, boundingVolume);
}

} // namespace Cesium3DTiles
