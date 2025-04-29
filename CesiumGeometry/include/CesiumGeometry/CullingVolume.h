#pragma once

#include <CesiumGeometry/Plane.h>

#include <glm/ext/matrix_double4x4.hpp>

namespace CesiumGeometry {

/**
 * @brief A culling volume, defined by four planes.
 *
 * The planes describe the culling volume that may be created for
 * the view frustum of a camera. The normals of these planes will
 * point inwards.
 */
struct CullingVolume final {

  /**
   * @brief The left plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane leftPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};

  /**
   * @brief The right plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane rightPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};

  /**
   * @brief The top plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane topPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};

  /**
   * @brief The bottom plane of the culling volume.
   *
   * Defaults to (0,0,1), with a distance of 0.
   */
  CesiumGeometry::Plane bottomPlane{glm::dvec3(0.0, 0.0, 1.0), 0.0};
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
CullingVolume createCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double fovx,
    double fovy) noexcept;

/**
 * @brief Create a {@link CullingVolume} from a projection matrix.
 *
 * The matrix can be a composite view - projection matrix; the volume will then
 * cull world coordinates. It can also be a model - view - projection matrix,
 * which will cull model coordinates.
 */
CullingVolume createCullingVolume(const glm::dmat4& clipMatrix);

/**
 * @brief Creates a {@link CullingVolume} for a perspective frustum.
 *
 * @param position The eye position
 * @param direction The viewing direction
 * @param up The up-vector of the frustum
 * @param l left edge
 * @param r right edge
 * @param t top edge
 * @param b bottom edge
 * @param n near plane distance
 * @return The {@link CullingVolume}
 */
CullingVolume createCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double l,
    double r,
    double b,
    double t,
    double n) noexcept;

/**
 * @brief Creates a {@link CullingVolume} for an orthographic frustum.
 *
 * @param position The eye position
 * @param direction The viewing direction
 * @param up The up-vector of the frustum
 * @param l left edge
 * @param r right edge
 * @param t top edge
 * @param b bottom edge
 * @param n near plane distance
 * @return The {@link CullingVolume}
 */

CullingVolume createOrthographicCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double l,
    double r,
    double b,
    double t,
    double n) noexcept;

} // namespace CesiumGeometry
