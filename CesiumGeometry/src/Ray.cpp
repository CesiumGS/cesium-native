#include "CesiumGeometry/Ray.h"
#include "CesiumUtility/Math.h"
#include <glm/geometric.hpp>
#include <stdexcept>

using namespace CesiumUtility;

namespace CesiumGeometry {

  Ray Ray::createUnchecked(const glm::dvec3& origin, const glm::dvec3& direction) noexcept {
    return Ray(origin, direction);
  }
  std::optional<Ray> Ray::createOptional(const glm::dvec3& origin, const glm::dvec3& direction) noexcept {
    if (!Math::equalsEpsilon(glm::length(direction), 1.0, Math::EPSILON6)) {
      return std::nullopt;
    }
    return Ray(origin, direction);
  }

  Ray Ray::createThrowing(const glm::dvec3& origin, const glm::dvec3& direction) {
    if (!Math::equalsEpsilon(glm::length(direction), 1.0, Math::EPSILON6)) {
      throw std::invalid_argument("direction must be normalized.");
    }
    return Ray(origin, direction);
  }


Ray::Ray(const glm::dvec3& origin, const glm::dvec3& direction)
    : _origin(origin), _direction(direction) {
}

Ray Ray::operator-() const noexcept {
  return Ray(this->_origin, -this->_direction);
}

} // namespace CesiumGeometry
