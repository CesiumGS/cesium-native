#include "CesiumGeometry/IntersectionTests.h"

#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"

#include <CesiumUtility/Math.h>

#include <glm/geometric.hpp>

using namespace CesiumUtility;

namespace CesiumGeometry {

/*static*/ std::optional<glm::dvec3>
IntersectionTests::rayPlane(const Ray& ray, const Plane& plane) noexcept {
  const double denominator = glm::dot(plane.getNormal(), ray.getDirection());

  if (glm::abs(denominator) < Math::Epsilon15) {
    // Ray is parallel to plane.  The ray may be in the polygon's plane.
    return std::optional<glm::dvec3>();
  }

  const double t =
      (-plane.getDistance() - glm::dot(plane.getNormal(), ray.getOrigin())) /
      denominator;

  if (t < 0) {
    return std::optional<glm::dvec3>();
  }

  return ray.getOrigin() + ray.getDirection() * t;
}

std::optional<glm::dvec3> IntersectionTests::rayTriangle(
    const Ray& ray,
    const glm::dvec3& V0,
    const glm::dvec3& V1,
    const glm::dvec3& V2,
    bool cullBackFaces) {
  double t;
  if (rayTriangleParametric(ray, V0, V1, V2, t, cullBackFaces)) {
    return std::make_optional<glm::dvec3>(
        ray.getOrigin() + t * ray.getDirection());
  } else {
    return std::optional<glm::dvec3>();
  }
}

bool IntersectionTests::rayTriangleParametric(
    const Ray& ray,
    const glm::dvec3& p0,
    const glm::dvec3& p1,
    const glm::dvec3& p2,
    double& t,
    bool cullBackFaces) {

  const glm::dvec3& origin = ray.getOrigin();
  const glm::dvec3& direction = ray.getDirection();

  glm::dvec3 edge0 = p1 - p0;
  glm::dvec3 edge1 = p2 - p0;

  glm::dvec3 p = glm::cross(direction, edge1);
  double det = glm::dot(edge0, p);
  if (cullBackFaces) {
    if (det < Math::Epsilon6)
      return false;

    glm::dvec3 tvec = origin - p0;
    double u = glm::dot(tvec, p);
    if (u < 0.0 || u > det)
      return false;

    glm::dvec3 q = glm::cross(tvec, edge0);
    double v = glm::dot(direction, q);
    if (v < 0.0 || u + v > det)
      return false;

    t = glm::dot(edge1, q);
    if (t < 0) {
      return false;
    }
    t /= det;

  } else {

    if (std::abs(det) < Math::Epsilon6)
      return false;

    double invDet = 1.0 / det;

    glm::dvec3 tvec = origin - p0;
    double u = glm::dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0)
      return false;

    glm::dvec3 q = glm::cross(tvec, edge0);
    double v = glm::dot(direction, q) * invDet;
    if (v < 0.0 || u + v > 1.0)
      return false;

    t = glm::dot(edge1, q) * invDet;
    if (t < 0.0)
      return false;
  }
  return true;
}

bool IntersectionTests::pointInTriangle2D(
    const glm::dvec2& point,
    const glm::dvec2& triangleVertA,
    const glm::dvec2& triangleVertB,
    const glm::dvec2& triangleVertC) noexcept {

  const glm::dvec2 ab = triangleVertB - triangleVertA;
  const glm::dvec2 ab_perp(-ab.y, ab.x);
  const glm::dvec2 bc = triangleVertC - triangleVertB;
  const glm::dvec2 bc_perp(-bc.y, bc.x);
  const glm::dvec2 ca = triangleVertA - triangleVertC;
  const glm::dvec2 ca_perp(-ca.y, ca.x);

  const glm::dvec2 av = point - triangleVertA;
  const glm::dvec2 cv = point - triangleVertC;

  const double v_proj_ab_perp = glm::dot(av, ab_perp);
  const double v_proj_bc_perp = glm::dot(cv, bc_perp);
  const double v_proj_ca_perp = glm::dot(cv, ca_perp);

  // This will determine in or out, irrespective of winding.
  return (v_proj_ab_perp >= 0.0 && v_proj_ca_perp >= 0.0 &&
          v_proj_bc_perp >= 0.0) ||
         (v_proj_ab_perp <= 0.0 && v_proj_ca_perp <= 0.0 &&
          v_proj_bc_perp <= 0.0);
}

} // namespace CesiumGeometry
