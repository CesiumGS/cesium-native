#pragma once

#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Library.h>

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeometry {

class Plane;

/**
 * @brief A bounding sphere with a center and a radius.
 */
class CESIUMGEOMETRY_API BoundingSphere final {
public:
  /**
   * @brief Construct a new instance.
   *
   * @param center The center of the bounding sphere.
   * @param radius The radius of the bounding sphere.
   */
  constexpr BoundingSphere(const glm::dvec3& center, double radius) noexcept
      : _center(center), _radius(radius) {}

  /**
   * @brief Gets the center of the bounding sphere.
   */
  constexpr const glm::dvec3& getCenter() const noexcept {
    return this->_center;
  }

  /**
   * @brief Gets the radius of the bounding sphere.
   */
  constexpr double getRadius() const noexcept { return this->_radius; }

  /**
   * @brief Determines on which side of a plane this boundings sphere is
   * located.
   *
   * @param plane The plane to test against.
   * @return The {@link CullingResult}:
   *  * `Inside` if the entire sphere is on the side of the plane the normal is
   * pointing.
   *  * `Outside` if the entire sphere is on the opposite side.
   *  * `Intersecting` if the sphere intersects the plane.
   */
  CullingResult intersectPlane(const Plane& plane) const noexcept;

  /**
   * @brief Computes the distance squared from a position to the closest point
   * on this bounding sphere. Returns 0 if the point is inside the sphere.
   *
   * @param position The position.
   * @return The distance squared from the position to the closest point on this
   * bounding sphere.
   *
   * @snippet TestBoundingSphere.cpp distanceSquaredTo
   */
  double
  computeDistanceSquaredToPosition(const glm::dvec3& position) const noexcept;

  /**
   * @brief Computes whether the given position is contained within the bounding
   * sphere.
   *
   * @param position The position.
   * @return Whether the position is contained within the bounding sphere.
   */
  bool contains(const glm::dvec3& position) const noexcept;

  /**
   * @brief Transforms this bounding sphere to another coordinate system using a
   * 4x4 matrix.
   *
   * If the transformation has non-uniform scale, the bounding sphere's radius
   * is scaled by the largest scale value among the transformation's axes.
   *
   * @param transformation The transformation.
   * @return The bounding sphere in the new coordinate system.
   */
  BoundingSphere transform(const glm::dmat4& transformation) const noexcept;

private:
  glm::dvec3 _center;
  double _radius;
};

} // namespace CesiumGeometry
