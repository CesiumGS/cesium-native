#pragma once

#include "CesiumGeometry/Plane.h"
#include <vector>

namespace Cesium3DTiles {

/**
 * @brief A culling volume, defined by a list of planes.
 *
 * The planes describe the culling volume that may be created for
 * the view frustum of a camera. The normals of these planes will
 * point inwards.
 */
class CullingVolume final {
private:
  /**
   * @brief The list of planes defining the culling volume.
   */
  const std::vector<CesiumGeometry::Plane> _planes;

public:
  /**
   * @brief Constructs an instance.
   *
   * @param planes The planes that will define this culling volume. They should
   * define a convex volume with their normals pointed inward.
   */
  CullingVolume(const std::vector<CesiumGeometry::Plane>& planes);

  /**
   * @brief Gets the planes that define this culling volume.
   */
  const std::vector<CesiumGeometry::Plane>& getPlanes() const noexcept {
    return this->_planes;
  }
};

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
CullingVolume createPerspectiveCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double fovx,
    double fovy) noexcept;

/**
 * @brief Creates a {@link CullingVolume} for a orthographic frustum.
 *
 * @param position The eye position
 * @param direction The viewing direction
 * @param up The up-vector of the frustum
 * @param orthographicWidth The width in meters of the frustum.
 * @param orthographicHeight The height in meters of the frustum.
 * @return The {@link CullingVolume}
 */
CullingVolume createOrthographicCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double orthographicWidth,
    double orthographicHeight) noexcept;
} // namespace Cesium3DTiles
