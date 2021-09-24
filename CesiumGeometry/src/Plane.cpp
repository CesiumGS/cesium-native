#include "CesiumGeometry/Plane.h"
#include "CesiumUtility/Math.h"
#include <glm/geometric.hpp>
#include <stdexcept>

using namespace CesiumUtility;

namespace CesiumGeometry {

Plane::Plane(const glm::dvec3& normal, double distance)
    : _normal(normal), _distance(distance) {
  //>>includeStart('debug', pragmas.debug);
  if (!Math::equalsEpsilon(glm::length(normal), 1.0, Math::EPSILON6)) {
    throw std::invalid_argument("normal must be normalized.");
  }
  //>>includeEnd('debug');
}

Plane::Plane(const glm::dvec3& point, const glm::dvec3& normal)
    : Plane(normal, -glm::dot(normal, point)) {}

double Plane::getPointDistance(const glm::dvec3& point) const noexcept {
  return glm::dot(this->_normal, point) + this->_distance;
}

glm::dvec3
Plane::projectPointOntoPlane(const glm::dvec3& point) const noexcept {
  // projectedPoint = point - (normal.point + scale) * normal
  const double pointDistance = this->getPointDistance(point);
  const glm::dvec3 scaledNormal = this->_normal * pointDistance;
  return point - scaledNormal;
}

} // namespace CesiumGeometry
