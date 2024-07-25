#include "CesiumGeometry/Ray.h"

#include <CesiumUtility/Math.h>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <stdexcept>

using namespace CesiumUtility;

namespace CesiumGeometry {

Ray::Ray(const glm::dvec3& origin, const glm::dvec3& direction)
    : _origin(origin), _direction(direction) {
  //>>includeStart('debug', pragmas.debug);
  if (!Math::equalsEpsilon(glm::length(direction), 1.0, Math::Epsilon6)) {
    throw std::invalid_argument("direction must be normalized.");
  }
  //>>includeEnd('debug');
}

glm::dvec3 Ray::pointFromDistance(double distance) const noexcept {
  return this->_origin + distance * this->_direction;
}

Ray Ray::transform(const glm::dmat4x4& transformation) const noexcept {
  return Ray(
      glm::dvec3(transformation * glm::dvec4(this->_origin, 1.0)),
      glm::normalize(
          glm::dvec3(transformation * glm::dvec4(this->_direction, 0.0))));
}

Ray Ray::operator-() const noexcept {
  return Ray(this->_origin, -this->_direction);
}

} // namespace CesiumGeometry
