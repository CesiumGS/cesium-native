#include "CesiumGeometry/BoundingCylinder.h"

#include "CesiumGeometry/CullingResult.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeometry/Plane.h"

#include <glm/glm.hpp>

namespace CesiumGeometry {

CullingResult
BoundingCylinder::intersectPlane(const Plane& plane) const noexcept {
  return this->_box.intersectPlane(plane);
}

double BoundingCylinder::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  return this->_box.computeDistanceSquaredToPosition(position);
}

bool BoundingCylinder::contains(const glm::dvec3& position) const noexcept {
  return this->_box.contains(position);
}

BoundingCylinder
BoundingCylinder::transform(const glm::dmat4& transformation) const noexcept {
  return BoundingCylinder(
      glm::dvec3(transformation * glm::dvec4(this->_box.getCenter(), 1.0)),
      glm::dmat3(transformation) * this->_box.getHalfAxes());
}

} // namespace CesiumGeometry
