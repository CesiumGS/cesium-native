#include "CesiumGeometry/BoundingSphere.h"

#include "CesiumGeometry/Plane.h"

#include <glm/geometric.hpp>

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
  const glm::dvec3 diff = this->_center - position;
  auto distance = glm::length(diff) - this->_radius;
  if (distance <= 0) {
    return 0;
  }
  return distance * distance;
}

} // namespace CesiumGeometry
