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
   * @brief Determines whether the point is completely inside the triangle.
   *
   * @param point The point to check.
   * @param triangleVertA The first vertex of the triangle.
   * @param triangleVertB The second vertex of the triangle.
   * @param triangleVertC The third vertex of the triangle.
   * @return Whether the point is within the triangle.
   */
  static bool pointInTriangle2D(
      const glm::dvec2& point,
      const glm::dvec2& triangleVertA,
      const glm::dvec2& triangleVertB,
      const glm::dvec2& triangleVertC) noexcept;
};

} // namespace CesiumGeometry
