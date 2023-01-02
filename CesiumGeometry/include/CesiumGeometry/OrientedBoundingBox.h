#pragma once

#include "CullingResult.h"
#include "Library.h"

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeometry {

class Plane;

/**
 * @brief A bounding volume defined as a closed and convex cuboid with any
 * orientation.
 *
 * @see BoundingSphere
 * @see BoundingRegion
 */
class CESIUMGEOMETRY_API OrientedBoundingBox final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param center The center of the box.
   * @param halfAxes The three orthogonal half-axes of the bounding box.
   * Equivalently, the transformation matrix to rotate and scale a 0x0x0 cube
   * centered at the origin.
   *
   * @snippet TestOrientedBoundingBox.cpp Constructor
   */
  OrientedBoundingBox(
      const glm::dvec3& center,
      const glm::dmat3& halfAxes) noexcept
      : _center(center),
        _halfAxes(halfAxes),
        // TODO: what should we do if halfAxes is singular?
        _inverseHalfAxes(glm::inverse(halfAxes)),
        _lengths(
            2.0 * glm::length(_halfAxes[0]),
            2.0 * glm::length(_halfAxes[1]),
            2.0 * glm::length(_halfAxes[2])) {}

  /**
   * @brief Gets the center of the box.
   */
  constexpr const glm::dvec3& getCenter() const noexcept {
    return this->_center;
  }

  /**
   * @brief Gets the transformation matrix, to rotate and scale the box to the
   * right position and size.
   */
  constexpr const glm::dmat3& getHalfAxes() const noexcept {
    return this->_halfAxes;
  }

  /**
   * @brief Gets the inverse transformation matrix, to rotate from world space
   * to local space relative to the box.
   */
  constexpr const glm::dmat3& getInverseHalfAxes() const noexcept {
    return this->_inverseHalfAxes;
  }

  /**
   * @brief Gets the lengths of the box on each local axis respectively.
   */
  constexpr const glm::dvec3& getLengths() const noexcept {
    return this->_lengths;
  }

  /**
   * @brief Determines on which side of a plane the bounding box is located.
   *
   * @param plane The plane to test against.
   * @return The {@link CullingResult}:
   *  * `Inside` if the entire box is on the side of the plane the normal is
   * pointing.
   *  * `Outside` if the entire box is on the opposite side.
   *  * `Intersecting` if the box intersects the plane.
   */
  CullingResult intersectPlane(const Plane& plane) const noexcept;

  /**
   * @brief Computes the distance squared from a given position to the closest
   * point on this bounding volume. The bounding volume and the position must be
   * expressed in the same coordinate system.
   *
   * @param position The position
   * @return The estimated distance squared from the bounding box to the point.
   *
   * @snippet TestOrientedBoundingBox.cpp distanceSquaredTo
   */
  double
  computeDistanceSquaredToPosition(const glm::dvec3& position) const noexcept;

  /**
   * @brief Computes whether the given position is contained within bounding
   * box.
   *
   * @param position The position.
   * @return Whether the position is contained within the bounding box.
   */
  bool contains(const glm::dvec3& position) const noexcept;

private:
  glm::dvec3 _center;
  glm::dmat3 _halfAxes;
  glm::dmat3 _inverseHalfAxes;
  glm::dvec3 _lengths;
};

} // namespace CesiumGeometry
