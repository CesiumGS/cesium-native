#pragma once

#include "Library.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace CesiumGeometry {
class Ray;
class Plane;

/**
 * @brief Functions for computing the intersection between geometries such as
 * rays, planes, triangles, and ellipsoids.
 */
class CESIUMGEOMETRY_API IntersectionTests final {
public:
  /**
   * @brief Computes the intersection of a ray and a plane.
   *
   * @param ray The ray.
   * @param plane The plane.
   * @return The point of intersection, or `std::nullopt` if there is no
   * intersection.
   */
  static std::optional<glm::dvec3>
  rayPlane(const Ray& ray, const Plane& plane) noexcept;

  /**
   * @brief Computes the intersection of a ray and an ellipsoid with the given
   * radii, centered at the origin.
   *
   * Returns false if any of the radii are 0.
   *
   * @param ray The ray.
   * @param radii The radii of ellipsoid.
   * @return The distances along the ray where it intersects the ellipsoid, or
   * `std::nullopt` if there are no intersections. X is the entry, and y is the
   * exit.
   *
   */
  static std::optional<glm::dvec2>
  rayEllipsoid(const Ray& ray, const glm::dvec3& radii) noexcept;

  /**
   * @brief Determines whether a given point is completely inside a triangle
   * defined by three 2D points.
   *
   * This function does not check for degeneracy of the triangle.
   *
   * @param point The point to check.
   * @param triangleVertA The first vertex of the triangle.
   * @param triangleVertB The second vertex of the triangle.
   * @param triangleVertC The third vertex of the triangle.
   * @return Whether the point is within the triangle.
   */
  static bool pointInTriangle(
      const glm::dvec2& point,
      const glm::dvec2& triangleVertA,
      const glm::dvec2& triangleVertB,
      const glm::dvec2& triangleVertC) noexcept;

  /**
   * @brief Determines whether a given point is completely inside a triangle
   * defined by three 3D points.
   *
   * Returns false for degenerate triangles.
   *
   * @param point The point to check.
   * @param triangleVertA The first vertex of the triangle.
   * @param triangleVertB The second vertex of the triangle.
   * @param triangleVertC The third vertex of the triangle.
   * @return Whether the point is within the triangle.
   */
  static bool pointInTriangle(
      const glm::dvec3& point,
      const glm::dvec3& triangleVertA,
      const glm::dvec3& triangleVertB,
      const glm::dvec3& triangleVertC) noexcept;

  /**
   * @brief Determines whether the point is completely inside a triangle defined
   * by three 3D points. If the point is inside, this also outputs the
   * barycentric coordinates for the point.
   *
   * Returns false for degenerate triangles.
   *
   * @param point The point to check.
   * @param triangleVertA The first vertex of the triangle.
   * @param triangleVertB The second vertex of the triangle.
   * @param triangleVertC The third vertex of the triangle.
   * @return Whether the point is within the triangle.
   */
  static bool pointInTriangle(
      const glm::dvec3& point,
      const glm::dvec3& triangleVertA,
      const glm::dvec3& triangleVertB,
      const glm::dvec3& triangleVertC,
      glm::dvec3& barycentricCoordinates) noexcept;
};

} // namespace CesiumGeometry
