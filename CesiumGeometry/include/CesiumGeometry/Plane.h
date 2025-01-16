#pragma once

#include <CesiumGeometry/Library.h>

#include <glm/vec3.hpp>

namespace CesiumGeometry {

/**
 * @brief A plane in Hessian Normal Format.
 */
class CESIUMGEOMETRY_API Plane final {
public:
  /**
   * @brief The XY plane passing through the origin, with normal in positive Z.
   */
  static const Plane ORIGIN_XY_PLANE;

  /**
   * @brief The YZ plane passing through the origin, with normal in positive X.
   */
  static const Plane ORIGIN_YZ_PLANE;

  /**
   * @brief The ZX plane passing through the origin, with normal in positive Y.
   */
  static const Plane ORIGIN_ZX_PLANE;

  /**
   * @brief Constructs a new plane with a +Z normal and a distance of 0.0.
   */
  Plane() noexcept;

  /**
   * @brief Constructs a new plane from a normal and a distance from the origin.
   *
   * The plane is defined by:
   * ```
   * ax + by + cz + d = 0
   * ```
   * where (a, b, c) is the plane's `normal`, d is the signed
   * `distance` to the plane, and (x, y, z) is any point on
   * the plane.
   *
   * @param normal The plane's normal (normalized).
   * @param distance The shortest distance from the origin to the plane. The
   * sign of `distance` determines which side of the plane the origin is on. If
   * `distance` is positive, the origin is in the half-space in the direction of
   * the normal; if negative, the origin is in the half-space opposite to the
   * normal; if zero, the plane passes through the origin.
   *
   * @exception std::exception `normal` must be normalized.
   *
   * Example:
   * @snippet TestPlane.cpp constructor-normal-distance
   */
  Plane(const glm::dvec3& normal, double distance);

  /**
   * @brief Construct a new plane from a point in the plane and the plane's
   * normal.
   *
   * @param point The point on the plane.
   * @param normal The plane's normal (normalized).
   *
   * @exception std::exception `normal` must be normalized.
   *
   * Example:
   * @snippet TestPlane.cpp constructor-point-normal
   */
  Plane(const glm::dvec3& point, const glm::dvec3& normal);

  /**
   * @brief Gets the plane's normal.
   */
  const glm::dvec3& getNormal() const noexcept { return this->_normal; }

  /**
   * @brief Gets the signed shortest distance from the origin to the plane.
   * The sign of `distance` determines which side of the plane the origin
   * is on.  If `distance` is positive, the origin is in the half-space
   * in the direction of the normal; if negative, the origin is in the
   * half-space opposite to the normal; if zero, the plane passes through the
   * origin.
   */
  double getDistance() const noexcept { return this->_distance; }

  /**
   * @brief Computes the signed shortest distance of a point to this plane.
   * The sign of the distance determines which side of the plane the point
   * is on.  If the distance is positive, the point is in the half-space
   * in the direction of the normal; if negative, the point is in the half-space
   * opposite to the normal; if zero, the plane passes through the point.
   *
   * @param point The point.
   * @returns The signed shortest distance of the point to the plane.
   */
  double getPointDistance(const glm::dvec3& point) const noexcept;

  /**
   * @brief Projects a point onto this plane.
   * @param point The point to project onto the plane.
   * @returns The projected point.
   */
  glm::dvec3 projectPointOntoPlane(const glm::dvec3& point) const noexcept;

private:
  glm::dvec3 _normal;
  double _distance;
};
} // namespace CesiumGeometry
