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
