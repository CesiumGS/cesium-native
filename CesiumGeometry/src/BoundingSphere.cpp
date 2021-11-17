#include "CesiumGeometry/BoundingSphere.h"

#include "CesiumGeometry/Plane.h"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

namespace CesiumGeometry {

CullingResult
BoundingSphere::intersectPlane(const Plane& plane) const noexcept {
  const double distanceToPlane =
      glm::dot(plane.getNormal(), this->_center) + plane.getDistance();

  const double radius = this->_radius;
  if (distanceToPlane < -radius) {
    // The center point is negative side of the plane normal
    return CullingResult::Outside;
  }
  if (distanceToPlane < radius) {
    // The center point is positive side of the plane, but radius extends beyond
    // it; partial overlap
    return CullingResult::Intersecting;
  }
  return CullingResult::Inside;
}

double BoundingSphere::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  const glm::dvec3 diff = position - this->_center;
  return glm::dot(diff, diff) - this->_radius * this->_radius;
}

/*static*/ constexpr BoundingSphere
BoundingSphere::fromPoints(const gsl::span<glm::dvec3>& points) noexcept {
  size_t pointCount = points.size();
  if (pointCount == 0) {
    return BoundingSphere(glm::dvec3(0.0), 0.0);
  }

  glm::dvec3 point0 = points[0];
  glm::dvec3 xMin = point0;
  glm::dvec3 yMin = point0;
  glm::dvec3 zMin = point0;
  glm::dvec3 xMax = point0;
  glm::dvec3 yMax = point0;
  glm::dvec3 zMax = point0;

  for (size_t i = 1; i < pointCount; i++) {
    glm::dvec3 point = points[i];

    // Store points containing the the smallest and largest components.
    if (point.x < xMin.x) {
      xMin = point;
    }

    if (point.x > xMax.x) {
      xMax = point;
    }

    if (point.y < yMin.y) {
      yMin = point;
    }

    if (point.y > yMax.y) {
      yMax = point;
    }

    if (point.z < zMin.z) {
      zMin = point;
    }

    if (point.z > zMax.z) {
      zMax = point;
    }
  }

  // Compute x, y, and z spans (Squared distances between each component's min
  // and max).
  double xSpanSquared = glm::distance2(xMax, xMin);
  double ySpanSquared = glm::distance2(yMax, yMin);
  double zSpanSquared = glm::distance2(zMax, zMin);

  // Set the diameter endpoints to the largest span.
  glm::dvec3 diameter1 = xMin;
  glm::dvec3 diameter2 = xMax;
  double maxSpanSquared = xSpanSquared;

  if (ySpanSquared > maxSpanSquared) {
    maxSpanSquared = ySpanSquared;
    diameter1 = yMin;
    diameter2 = yMax;
  }

  if (zSpanSquared > maxSpanSquared) {
    maxSpanSquared = zSpanSquared;
    diameter1 = zMin;
    diameter2 = zMax;
  }

  // Calculate the initial sphere found by Ritter's algorithm.
  glm::dvec3 ritterCenter = 0.5 * (diameter1 + diameter2);
  double ritterRadiusSquared = glm::distance2(diameter2, ritterCenter);
  double ritterRadius = glm::sqrt(ritterRadiusSquared);

  // Find the center of the sphere found using the Naive method.
  glm::dvec3 minBoxPt(xMin.x, yMin.y, zMin.z);
  glm::dvec3 maxBoxPt(xMax.x, yMax.y, zMax.z);
  glm::dvec3 naiveCenter = 0.5 * (minBoxPt + maxBoxPt);

  // Begin 2nd pass to find naive radius and modify the Ritter sphere.
  double naiveRadiusSquared = 0;
  for (size_t i = 0; i < pointCount; i++) {
    glm::dvec3 point = points[i];

    // Find the furthest point from the naive center to calculate the naive
    // radius.
    double naiveDistanceSquared = glm::distance2(point, naiveCenter);
    if (naiveDistanceSquared > naiveRadiusSquared) {
      naiveRadiusSquared = naiveDistanceSquared;
    }

    // Make adjustments to the Ritter sphere if any points are outside of it.
    glm::dvec3 ritterDifference = point - ritterCenter;
    double ritterDistanceSquared = glm::length2(ritterDifference);

    if (ritterDistanceSquared > ritterRadiusSquared) {
      double ritterDistance = glm::sqrt(ritterDistanceSquared);

      // Shift the Ritter center partway towards the point and expand its
      // radius to encompass all points
      double shiftAmount = 0.5 * (ritterDistance - ritterRadius);
      glm::dvec3 direction = ritterDifference / ritterDistance;
      ritterCenter += shiftAmount * direction;
      ritterRadius += shiftAmount;
    }
  }

  double naiveRadius = glm::sqrt(naiveRadiusSquared);
  if (ritterRadius < naiveRadius) {
    return BoundingSphere(ritterCenter, ritterRadius);
  } else {
    return BoundingSphere(naiveCenter, naiveRadius);
  }
}

} // namespace CesiumGeometry
