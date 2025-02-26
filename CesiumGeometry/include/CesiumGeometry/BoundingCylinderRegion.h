#pragma once

#include <CesiumGeometry/Library.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumUtility/Math.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeometry {

class Plane;
enum class CullingResult;

/**
 * @brief A bounding volume defined as a region following the surface of a
 * cylinder between two radius values. Used for creating bounding volumes
 * from `3DTILES_bounding_volume_cylinder`.
 *
 * Note: This uses a \ref OrientedBoundingBox underneath the hood to approximate
 * the result, similar to how CesiumJS approximates cylinders. The output may
 * not be accurate to the actual cylinder itself.
 *
 * @see OrientedBoundingBox
 */
class CESIUMGEOMETRY_API BoundingCylinderRegion final {
public:
  /**
   * @brief Construct a new bounding cylinder region.
   *
   * @remarks A cylinder region is defined relative to a reference cylinder
   * centered at the local origin. The height aligns with the z-axis, and the
   * cylinder extends to half the height in each direction. The angular bounds
   * are in the range [-pi, pi], and are oriented such that an angle of -pi
   * aligns with the negative y-axis, while an angle of 0 aligns with the
   * positive y-axis. The angular range opens counter-clockwise.
   *
   * It is possible for the region to only occupy part of the cylinder, and if
   * that is the case, the region's center may not necessarily equal the
   * translation. Additionally, the rotation is applied at to the reference
   * cylinder at the local origin. In other words, the region is rotated around
   * the whole cylinder's center, and not necessarily its own.
   *
   * @param translation The translation applied to the reference cylinder.
   * @param rotation The rotation applied to the reference cylinder.
   * @param height The height of the cylinder region along the z-axis.
   * @param radialBounds The radial bounds of the region, where x is the minimum
   * radius and y is the maximum.
   * @param angularBounds The angular bounds of the region, where x is the
   * minimum angle and y is the maximum.
   *
   * @snippet TestBoundingCylinderRegion.cpp Constructor
   */
  BoundingCylinderRegion(
      const glm::dvec3& translation,
      const glm::dquat& rotation,
      double height,
      const glm::dvec2& radialBounds,
      const glm::dvec2& angularBounds = {
          -CesiumUtility::Math::OnePi,
          CesiumUtility::Math::OnePi}) noexcept;

  /**
   * @brief Gets the center of the cylinder region.
   */
  constexpr const glm::dvec3& getCenter() const noexcept {
    return this->_box.getCenter();
  }

  /**
   * @brief Gets the translation that is applied to the bounding cylinder
   * region.
   */
  constexpr const glm::dvec3& getTranslation() const noexcept {
    return this->_translation;
  }

  /**
   * @brief Gets the rotation that is applied to the bounding cylinder
   * region.
   */
  constexpr const glm::dquat& getRotation() const noexcept {
    return this->_rotation;
  }

  /**
   * @brief Gets the height of the cylinder region.
   */
  constexpr double getHeight() const noexcept { return this->_height; }

  /**
   * @brief Gets the radial bounds of the cylinder region.
   */
  constexpr const glm::dvec2& getRadialBounds() const noexcept {
    return this->_radialBounds;
  }

  /**
   * @brief Gets the angular bounds of the cylinder region.
   */
  constexpr const glm::dvec2& getAngularBounds() const noexcept {
    return this->_angularBounds;
  }

  /**
   * @brief Determines on which side of a plane the bounding cylinder is
   * located.
   *
   * @param plane The plane to test against.
   * @return The {@link CullingResult}:
   *  * `Inside` if the entire cylinder is on the side of the plane the normal
   * is pointing.
   *  * `Outside` if the entire cylinder is on the opposite side.
   *  * `Intersecting` if the cylinder intersects the plane.
   */
  CullingResult intersectPlane(const Plane& plane) const noexcept;

  /**
   * @brief Computes the distance squared from a given position to the closest
   * point on the bounding volume. The bounding volume and the position must be
   * expressed in the same coordinate system.
   *
   * @param position The position
   * @return The estimated distance squared from the bounding cylinder to the
   * point.
   */
  double
  computeDistanceSquaredToPosition(const glm::dvec3& position) const noexcept;

  /**
   * @brief Computes whether the given position is contained within the bounding
   * cylinder.
   *
   * @param position The position.
   * @return Whether the position is contained within the bounding cylinder.
   */
  bool contains(const glm::dvec3& position) const noexcept;

  /**
   * @brief Transforms this bounding cylinder region to another coordinate
   * system using a 4x4 matrix.
   *
   * @param transformation The transformation.
   * @return The bounding cylinder region in the new coordinate system.
   */
  BoundingCylinderRegion
  transform(const glm::dmat4& transformation) const noexcept;

  /**
   * @brief Converts this bounding cylinder region to an oriented bounding box.
   */
  OrientedBoundingBox toOrientedBoundingBox() const noexcept;

private:
  glm::dvec3 _translation;
  glm::dquat _rotation;
  double _height;
  glm::dvec2 _radialBounds;
  glm::dvec2 _angularBounds;

  /**
   * The oriented bounding box that is tightly-fitted around the region.
   * Used to approximate the region for most computations.
   */
  OrientedBoundingBox _box;
};

} // namespace CesiumGeometry
