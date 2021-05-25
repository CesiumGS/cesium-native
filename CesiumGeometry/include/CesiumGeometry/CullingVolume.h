#pragma once

#include "CesiumGeometry/Plane.h"

namespace Cesium3DTiles {

/**
 * @brief A culling volume, defined by four planes.
 *
 * The planes describe the culling volume that may be created for
 * the view frustum of a camera. The normals of these planes will
 * point inwards.
 */
struct CullingVolume final {

  /**
   * @brief Creates a {@link CullingVolume} for a perspective frustum.
   *
   * @param position The eye position
   * @param direction The viewing direction
   * @param up The up-vector of the frustum
   * @param fovx The horizontal Field-Of-View angle, in radians
   * @param fovy The vertical Field-Of-View angle, in radians
   * @return The {@link CullingVolume}
   */
  static CullingVolume createUnchecked(
      const glm::dvec3& position,
      const glm::dvec3& direction,
      const glm::dvec3& up,
      double fovx,
      double fovy) noexcept;

  /**
   * @brief The left plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane leftPlane =
      CesiumGeometry::Plane::createUnchecked(glm::dvec3(0.0, 0.0, 1.0), 0.0);

  /**
   * @brief The right plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane rightPlane =
      CesiumGeometry::Plane::createUnchecked(glm::dvec3(0.0, 0.0, 1.0), 0.0);

  /**
   * @brief The top plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane topPlane =
      CesiumGeometry::Plane::createUnchecked(glm::dvec3(0.0, 0.0, 1.0), 0.0);

  /**
   * @brief The bottom plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane bottomPlane =
      CesiumGeometry::Plane::createUnchecked(glm::dvec3(0.0, 0.0, 1.0), 0.0);

private:
  CullingVolume();
};

} // namespace Cesium3DTiles
