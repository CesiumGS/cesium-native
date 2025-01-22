#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/detail/setup.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/matrix.hpp>

#include <cmath>
#include <limits>
#include <optional>
#include <utility>

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

std::optional<glm::dvec2> IntersectionTests::rayEllipsoid(
    const Ray& ray,
    const glm::dvec3& radii) noexcept {
  if (radii.x == 0.0 || radii.y == 0.0 || radii.z == 0.0) {
    return std::nullopt;
  }

  glm::dvec3 inverseRadii = 1.0 / radii;

  glm::dvec3 origin = ray.getOrigin();
  glm::dvec3 direction = ray.getDirection();

  glm::dvec3 q = inverseRadii * origin;
  glm::dvec3 w = inverseRadii * direction;

  const double q2 = pow(length(q), 2);
  const double qw = dot(q, w);

  double difference, w2, product, discriminant, temp;
  if (q2 > 1.0) {
    // Outside ellipsoid.
    if (qw >= 0.0) {
      // Looking outward or tangent (0 intersections).
      return std::optional<glm::dvec2>();
    }

    // qw < 0.0.
    const double qw2 = qw * qw;
    difference = q2 - 1.0; // Positively valued.
    w2 = pow(length(w), 2);
    product = w2 * difference;

    if (qw2 < product) {
      // Imaginary roots (0 intersections).
      return std::optional<glm::dvec2>();
    }
    if (qw2 > product) {
      // Distinct roots (2 intersections).
      discriminant = qw * qw - product;
      temp = -qw + sqrt(discriminant); // Avoid cancellation.
      const double root0 = temp / w2;
      const double root1 = difference / temp;
      if (root0 < root1) {
        return glm::dvec2(root0, root1);
      }

      return glm::dvec2(root1, root0);
    }
    // qw2 == product.  Repeated roots (2 intersections).
    const double root = sqrt(difference / w2);
    return glm::dvec2(root, root);
  }
  if (q2 < 1.0) {
    // Inside ellipsoid (2 intersections).
    difference = q2 - 1.0; // Negatively valued.
    w2 = pow(length(w), 2);
    product = w2 * difference; // Negatively valued.

    discriminant = qw * qw - product;
    temp = -qw + sqrt(discriminant); // Positively valued.
    return glm::dvec2(0.0, temp / w2);
  }
  // q2 == 1.0. On ellipsoid.
  if (qw < 0.0) {
    // Looking inward.
    w2 = pow(length(w), 2);
    return glm::dvec2(0.0, -qw / w2);
  }
  // qw >= 0.0.  Looking outward or tangent.
  return std::optional<glm::dvec2>();
}

std::optional<glm::dvec3> IntersectionTests::rayTriangle(
    const Ray& ray,
    const glm::dvec3& v0,
    const glm::dvec3& v1,
    const glm::dvec3& v2,
    bool cullBackFaces) {
  std::optional<double> t =
      rayTriangleParametric(ray, v0, v1, v2, cullBackFaces);
  if (t && t.value() >= 0)
    return std::make_optional<glm::dvec3>(ray.pointFromDistance(t.value()));
  else
    return std::nullopt;
}

std::optional<double> IntersectionTests::rayTriangleParametric(
    const Ray& ray,
    const glm::dvec3& p0,
    const glm::dvec3& p1,
    const glm::dvec3& p2,
    bool cullBackFaces) {

  const glm::dvec3& origin = ray.getOrigin();
  const glm::dvec3& direction = ray.getDirection();

  glm::dvec3 edge0 = p1 - p0;
  glm::dvec3 edge1 = p2 - p0;

  glm::dvec3 p = glm::cross(direction, edge1);
  double det = glm::dot(edge0, p);
  if (cullBackFaces) {
    if (det < Math::Epsilon8)
      return std::nullopt;

    glm::dvec3 tvec = origin - p0;
    double u = glm::dot(tvec, p);
    if (u < 0.0 || u > det)
      return std::nullopt;

    glm::dvec3 q = glm::cross(tvec, edge0);
    double v = glm::dot(direction, q);
    if (v < 0.0 || u + v > det)
      return std::nullopt;

    return glm::dot(edge1, q) / det;

  } else {

    if (glm::abs(det) < Math::Epsilon8)
      return std::nullopt;

    double invDet = 1.0 / det;

    glm::dvec3 tvec = origin - p0;
    double u = glm::dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0)
      return std::nullopt;

    glm::dvec3 q = glm::cross(tvec, edge0);
    double v = glm::dot(direction, q) * invDet;
    if (v < 0.0 || u + v > 1.0)
      return std::nullopt;

    return glm::dot(edge1, q) * invDet;
  }
}

std::optional<glm::dvec3>
IntersectionTests::rayAABB(const Ray& ray, const AxisAlignedBox& aabb) {
  std::optional<double> t = rayAABBParametric(ray, aabb);

  if (t && t.value() >= 0)
    return std::make_optional<glm::dvec3>(ray.pointFromDistance(t.value()));
  else
    return std::nullopt;
}

std::optional<double> IntersectionTests::rayAABBParametric(
    const Ray& ray,
    const AxisAlignedBox& aabb) {
  const glm::dvec3& dir = ray.getDirection();
  const glm::dvec3& origin = ray.getOrigin();
  const glm::dvec3* min = reinterpret_cast<const glm::dvec3*>(&aabb.minimumX);
  const glm::dvec3* max = reinterpret_cast<const glm::dvec3*>(&aabb.maximumX);
  double greatestMin = -std::numeric_limits<double>::max();
  double smallestMax = std::numeric_limits<double>::max();
  double tmin = greatestMin;
  double tmax = smallestMax;

  for (glm::length_t i = 0; i < 3; ++i) {
    if (glm::abs(dir[i]) < Math::Epsilon6) {
      continue;
    } else {
      tmin = ((*min)[i] - origin[i]) / dir[i];
      tmax = ((*max)[i] - origin[i]) / dir[i];
    }
    if (tmin > tmax) {
      std::swap(tmin, tmax);
    }
    greatestMin = glm::max(tmin, greatestMin);
    smallestMax = glm::min(tmax, smallestMax);
  }
  if (smallestMax < 0.0 || greatestMin > smallestMax) {
    return std::nullopt;
  }
  return greatestMin < 0.0 ? smallestMax : greatestMin;
}

std::optional<glm::dvec3>
IntersectionTests::rayOBB(const Ray& ray, const OrientedBoundingBox& obb) {
  std::optional<double> t = rayOBBParametric(ray, obb);
  if (t && t.value() >= 0)
    return std::make_optional<glm::dvec3>(ray.pointFromDistance(t.value()));
  else
    return std::nullopt;
}

std::optional<double> IntersectionTests::rayOBBParametric(
    const Ray& ray,
    const OrientedBoundingBox& obb) {
  // Extract the rotation from the OBB's rotatin/scale transformation and
  // invert it. This code assumes that there is not a negative scale, that
  // there's no skew, that there's no other funny business. Non-uniform scale
  // is fine!
  const glm::dmat3& halfAxes = obb.getHalfAxes();
  glm::dvec3 halfLengths = obb.getLengths() * 0.5;
  glm::dmat3 rotationOnly(
      halfAxes[0] / halfLengths.x,
      halfAxes[1] / halfLengths.y,
      halfAxes[2] / halfLengths.z);
  glm::dmat3 inverseRotation = glm::transpose(rotationOnly);

  // Find the equivalent ray in the coordinate system where the OBB is not
  // rotated or translated. That is, where it's an AABB at the origin.
  glm::dvec3 relativeOrigin = ray.getOrigin() - obb.getCenter();
  glm::dvec3 rayOrigin(inverseRotation * relativeOrigin);
  glm::dvec3 rayDirection(inverseRotation * ray.getDirection());

  // Find the distance to the new ray's intersection with the AABB, which is
  // equivalent to the distance of the original ray intersection with the OBB.
  glm::dvec3 ll = -halfLengths;
  glm::dvec3 ur = +halfLengths;
  AxisAlignedBox aabb(ll.x, ll.y, ll.z, ur.x, ur.y, ur.z);
  return rayAABBParametric(Ray(rayOrigin, rayDirection), aabb);
}

std::optional<glm::dvec3>
IntersectionTests::raySphere(const Ray& ray, const BoundingSphere& sphere) {
  std::optional<double> t = raySphereParametric(ray, sphere);
  if (t && t.value() >= 0)
    return std::make_optional<glm::dvec3>(ray.pointFromDistance(t.value()));
  else
    return std::nullopt;
}

namespace {
bool solveQuadratic(
    double a,
    double b,
    double c,
    double& root0,
    double& root1) {
  double det = b * b - 4.0 * a * c;
  if (det < 0.0) {
    return false;
  } else if (det > 0.0) {
    double denom = 1.0 / (2.0 * a);
    double disc = glm::sqrt(det);
    root0 = (-b + disc) * denom;
    root1 = (-b - disc) * denom;

    if (root1 < root0) {
      std::swap(root1, root0);
    }
    return true;
  }

  double root = -b / (2.0 * a);
  if (root == 0.0) {
    return false;
  }

  root0 = root1 = root;
  return true;
}
} // namespace

std::optional<double> IntersectionTests::raySphereParametric(
    const Ray& ray,
    const BoundingSphere& sphere) {
  const glm::dvec3& origin = ray.getOrigin();
  const glm::dvec3& direction = ray.getDirection();

  const glm::dvec3& center = sphere.getCenter();
  const double radiusSquared = sphere.getRadius() * sphere.getRadius();

  const glm::dvec3 diff = origin - center;

  const double b = 2.0 * glm::dot(direction, diff);
  const double c = glm::dot(diff, diff) - radiusSquared;

  double t0, t1;
  if (solveQuadratic(1.0, b, c, t0, t1)) {
    return t0 < 0 ? t1 : t0;
  }
  return std::nullopt;
}

bool IntersectionTests::pointInTriangle(
    const glm::dvec2& point,
    const glm::dvec2& triangleVertA,
    const glm::dvec2& triangleVertB,
    const glm::dvec2& triangleVertC) noexcept {
  // Define the triangle edges.
  const glm::dvec2 ab = triangleVertB - triangleVertA;
  const glm::dvec2 bc = triangleVertC - triangleVertB;
  const glm::dvec2 ca = triangleVertA - triangleVertC;

  // Get the vector perpendicular to each triangle edge in the same 2D plane).
  const glm::dvec2 ab_perp(-ab.y, ab.x);
  const glm::dvec2 bc_perp(-bc.y, bc.x);
  const glm::dvec2 ca_perp(-ca.y, ca.x);

  // Get vectors from triangle corners to point.
  const glm::dvec2 av = point - triangleVertA;
  const glm::dvec2 cv = point - triangleVertC;

  // Project the point vectors onto the perpendicular vectors.
  const double v_proj_ab_perp = glm::dot(av, ab_perp);
  const double v_proj_bc_perp = glm::dot(cv, bc_perp);
  const double v_proj_ca_perp = glm::dot(cv, ca_perp);

  // The projections determine in what direction the point is from the triangle
  // edge. This will determine in or out, irrespective of winding.
  return (v_proj_ab_perp >= 0.0 && v_proj_ca_perp >= 0.0 &&
          v_proj_bc_perp >= 0.0) ||
         (v_proj_ab_perp <= 0.0 && v_proj_ca_perp <= 0.0 &&
          v_proj_bc_perp <= 0.0);
}

bool IntersectionTests::pointInTriangle(
    const glm::dvec3& point,
    const glm::dvec3& triangleVertA,
    const glm::dvec3& triangleVertB,
    const glm::dvec3& triangleVertC) noexcept {
  // PERFORMANCE_IDEA: Investigate if there is a faster algorithm that can do
  // this test, but without needing to compute barycentric coordinates
  glm::dvec3 unused;
  return IntersectionTests::pointInTriangle(
      point,
      triangleVertA,
      triangleVertB,
      triangleVertC,
      unused);
}

bool IntersectionTests::pointInTriangle(
    const glm::dvec3& point,
    const glm::dvec3& triangleVertA,
    const glm::dvec3& triangleVertB,
    const glm::dvec3& triangleVertC,
    glm::dvec3& barycentricCoordinates) noexcept {
  // PERFORMANCE_IDEA: If optimization is necessary in the future, there are
  // algorithms that avoid length computations (which involve square root
  // operations).

  // Define the triangle edges.
  const glm::dvec3 ab = triangleVertB - triangleVertA;
  const glm::dvec3 bc = triangleVertC - triangleVertB;

  const glm::dvec3 triangleNormal = glm::cross(ab, bc);
  const double lengthSquared = glm::length2(triangleNormal);
  if (lengthSquared < CesiumUtility::Math::Epsilon8) {
    // Don't handle invalid triangles.
    return false;
  }

  // Technically this is the area of the parallelogram between the vectors,
  // but triangles are 1/2 of the area, and the division will cancel out later.
  const double triangleAreaInv = 1.0 / glm::sqrt(lengthSquared);

  const glm::dvec3 ap = point - triangleVertA;
  const double triangleABPRatio =
      glm::length(glm::cross(ab, ap)) * triangleAreaInv;
  if (triangleABPRatio > 1) {
    return false;
  }

  const glm::dvec3 bp = point - triangleVertB;
  const double triangleBCPRatio =
      glm::length(glm::cross(bc, bp)) * triangleAreaInv;
  if (triangleBCPRatio > 1) {
    return false;
  }

  const double triangleCAPRatio = 1.0 - triangleABPRatio - triangleBCPRatio;
  if (triangleCAPRatio < 0) {
    return false;
  }

  barycentricCoordinates.x = triangleBCPRatio;
  barycentricCoordinates.y = triangleCAPRatio;
  barycentricCoordinates.z = triangleABPRatio;

  return true;
}

} // namespace CesiumGeometry
