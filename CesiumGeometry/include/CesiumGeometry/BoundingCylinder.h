#pragma once

#include "CesiumGeometry/CullingResult.h"
#include "CesiumGeometry/Library.h"

#include <glm/glm.hpp>

namespace CesiumGeometry {

class Plane;

/**
 * @brief A bounding volume defined as a closed and convex cylinder with any
 * orientation.
 */
class CESIUMGEOMETRY_API BoundingCylinder final {
public:
  /**
   * @brief Construct a new instance.
   *
   * @param center The center of the cylinder.
   * @param halfAxes The three orthogonal half-axes of the bounding cylinder.
   * Equivalently, the transformation matrix to rotate and scale a cylinder with
   * a radius and height of 1 that is centered at the origin.
   *
   * @snippet TestBoundingCylinder.cpp Constructor
   */
  BoundingCylinder(
      const glm::dvec3& center,
      const glm::dmat3& halfAxes) noexcept
      : _center(center),
        _halfAxes(halfAxes),
        // TODO: what should we do if halfAxes is singular?
        _inverseHalfAxes(glm::inverse(halfAxes)),
        _radius(glm::length(halfAxes[0])),
        _height(2.0 * glm::length(halfAxes[2])) {}

  /**
   * @brief Gets the center of the cylinder.
   */
  constexpr const glm::dvec3& getCenter() const noexcept {
    return this->_center;
  }

  /**
   * @brief Gets the three orthogonal half-axes of the cylinder.
   * Equivalently, the transformation matrix to rotate and scale a cylinder with
   * a radius and height of 1 that is centered at the origin.
   */
  constexpr const glm::dmat3& getHalfAxes() const noexcept {
    return this->_halfAxes;
  }

  /**
   * @brief Gets the radius of the cylinder.
   */
  constexpr double getRadius() const noexcept { return this->_radius; }

  /**
   * @brief Gets the height of the cylinder.
   */
  constexpr double getHeight() const noexcept { return this->_height; }

  /**
   * @brief Gets the inverse transformation matrix, to rotate from world space
   * to local space relative to the cylinder.
   */
  constexpr const glm::dmat3& getInverseHalfAxes() const noexcept {
    return this->_inverseHalfAxes;
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
   * @brief Transforms this bounding cylinder to another coordinate system using
   * a 4x4 matrix.
   *
   * @param transformation The transformation.
   * @return The bounding cylinder in the new coordinate system.
   */
  BoundingCylinder transform(const glm::dmat4& transformation) const noexcept;

private:
  glm::dvec3 _center;
  glm::dmat3 _halfAxes;
  glm::dmat3 _inverseHalfAxes;

  double _radius;
  double _height;
};

} // namespace CesiumGeometry
