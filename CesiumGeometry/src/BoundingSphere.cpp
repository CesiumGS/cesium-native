#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
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

bool BoundingSphere::contains(const glm::dvec3& position) const noexcept {
  return glm::distance(this->_center, position) <= this->_radius;
}

BoundingSphere
BoundingSphere::transform(const glm::dmat4& transformation) const noexcept {
  const glm::dvec3 center =
      glm::dvec3(transformation * glm::dvec4(this->getCenter(), 1.0));

  const double uniformScale = glm::max(
      glm::max(
          glm::length(glm::dvec3(transformation[0])),
          glm::length(glm::dvec3(transformation[1]))),
      glm::length(glm::dvec3(transformation[2])));

  return BoundingSphere(center, this->getRadius() * uniformScale);
}

} // namespace CesiumGeometry
