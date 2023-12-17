#pragma once

#include "Library.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace CesiumGeometry {
class Ray;
class Plane;
struct AxisAlignedBox;
class OrientedBoundingBox;
class BoundingSphere;

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
   * @brief Tests if a ray hits a triangle and returns the hit point, if any.
   *
   * @param ray The ray.
   * @param p0 The first vertex of the triangle.
   * @param p1 The second vertex of the triangle.
   * @param p2 The third vertex of the triangle.
   * @param cullBackFaces Whether to cull back faces or not.
   * @return The point of intersection, or `std::nullopt` if there is no
   * intersection.
   */
  static std::optional<glm::dvec3> rayTriangle(
      const Ray& ray,
      const glm::dvec3& p0,
      const glm::dvec3& p1,
      const glm::dvec3& p2,
      bool cullBackFaces = false);

  static double rayTriangleParametric(
      const Ray& ray,
      const glm::dvec3& p0,
      const glm::dvec3& p1,
      const glm::dvec3& p2,
      bool cullBackFaces = false);

  /**
   * @brief Computes the intersection of a ray and an axis aligned bounding box.
   *
   * @param ray The ray.
   * @param aabb The axis aligned bounding box.
   * @return The point of intersection, or `std::nullopt` if there is no
   * intersection.
   */
  static std::optional<glm::dvec3>
  rayAABB(const Ray& ray, const AxisAlignedBox& aabb);

  static double rayAABBParametric(const Ray& ray, const AxisAlignedBox& aabb);

  /**
   * @brief Computes the intersection of a ray and an oriented bounding box.
   *
   * @param ray The ray.
   * @param obb The oriented bounding box.
   * @return The point of intersection, or `std::nullopt` if there is no
   * intersection.
   */
  static std::optional<glm::dvec3>
  rayOBB(const Ray& ray, const OrientedBoundingBox& obb);

  static double
  rayOBBParametric(const Ray& ray, const OrientedBoundingBox& obb);

  /**
   * @brief Computes the intersection of a ray and a bounding sphere.
   *
   * @param ray The ray.
   * @param sphere The bounding sphere.
   * @return The point of intersection, or `std::nullopt` if there is no
   * intersection.
   */
  static std::optional<glm::dvec3>
  raySphere(const Ray& ray, const BoundingSphere& sphere);

  static double
  raySphereParametric(const Ray& ray, const BoundingSphere& sphere);

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
