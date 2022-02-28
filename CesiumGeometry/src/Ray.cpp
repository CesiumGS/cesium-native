#include "CesiumGeometry/Ray.h"

#include <CesiumUtility/Math.h>

#include <glm/geometric.hpp>

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

Ray Ray::operator-() const noexcept {
  return Ray(this->_origin, -this->_direction);
}

} // namespace CesiumGeometry
