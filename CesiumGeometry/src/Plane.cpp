#include "CesiumGeometry/Plane.h"
#include "CesiumUtility/Math.h"
#include <glm/geometric.hpp>
#include <stdexcept>

using namespace CesiumUtility;

namespace CesiumGeometry {

Plane Plane::createUnchecked(const glm::dvec3& normal, double distance) noexcept {
  return Plane(normal, distance);
}

std::optional<Plane> Plane::createOptional(const glm::dvec3& normal, double distance) noexcept {
  if (!Math::equalsEpsilon(glm::length(normal), 1.0, Math::EPSILON6)) {
    return std::nullopt;
  }
  return Plane(normal, distance);
}

Plane Plane::createThrowing(const glm::dvec3& normal, double distance) {
  if (!Math::equalsEpsilon(glm::length(normal), 1.0, Math::EPSILON6)) {
    throw std::invalid_argument("normal must be normalized.");
  }
  return Plane(normal, distance);
}


Plane::Plane(const glm::dvec3& normal, double distance)
    : _normal(normal), _distance(distance) {
}

double Plane::getPointDistance(const glm::dvec3& point) const noexcept {
  return glm::dot(this->_normal, point) + this->_distance;
}

glm::dvec3
Plane::projectPointOntoPlane(const glm::dvec3& point) const noexcept {
  // projectedPoint = point - (normal.point + scale) * normal
  double pointDistance = this->getPointDistance(point);
  glm::dvec3 scaledNormal = this->_normal * pointDistance;
  return point - scaledNormal;
}

} // namespace CesiumGeometry
