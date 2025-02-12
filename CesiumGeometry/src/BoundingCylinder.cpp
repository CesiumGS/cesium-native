#include "CesiumGeometry/BoundingCylinder.h"

#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeometry/Plane.h"

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>

namespace CesiumGeometry {

CullingResult
BoundingCylinder::intersectPlane(const Plane& plane) const noexcept {
  OrientedBoundingBox obb = OrientedBoundingBox::fromCylinder(*this);
  return obb.intersectPlane(plane);
}

double BoundingCylinder::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  const glm::dmat3& halfAxes = this->_halfAxes;
  double radius = this->_radius;
  double height = this->_height;

  double r2 = radius * radius;
  glm::dvec3 positionToCenter = this->_center - position;

  // The line segment representing the height axis between the two ends of the
  // cylinder.
  glm::dvec3 heightAxis = 2.0 * halfAxes[2];
  // The unit vector along the height axis.
  glm::dvec3 u = glm::normalize(halfAxes[2]);
  // The distance along the height axis.
  double x = abs(glm::dot(positionToCenter, u));
  // The squared distance to the center of the cylinder.
  double d2 = glm::dot(positionToCenter, positionToCenter);
  // The squared *from* the height axis.
  double y2 = d2 - x * x;

  // Check if the projection of the position is within the cylinder's
  // height bounds.
  if (x <= 0.5 * height) {
    // If y2 <= r2, then the point is inside the cylinder.
    // Otherwise, the distance is the difference between y and r.
    return y2 <= r2 ? 0 : pow(sqrt(y2) - radius, 2);
  }

  // Otherwise, compute the distance from the point to the disc on the cylinder.
  double discDistanceSquared = pow(x - 0.5 * height, 2);
  if (y2 <= r2) {
    // True when the point projects perfectly onto the disc.
    return discDistanceSquared;
  }

  // The point's projection falls outside both the height and discs of the
  // cylinder. This distance is thus a sum of first and second case.
  return pow(sqrt(y2) - radius, 2) + discDistanceSquared;
}

bool BoundingCylinder::contains(const glm::dvec3& position) const noexcept {
  // This is the same as computeDistanceSquaredToPosition minus some extra
  // checks.
  const glm::dmat3& halfAxes = this->getHalfAxes();

  double r2 = this->_radius * this->_radius;
  glm::dvec3 positionToCenter = this->_center - position;

  // The line segment representing the height axis between the two ends of the
  // cylinder.
  glm::dvec3 heightAxis = 2.0 * halfAxes[2];
  // The unit vector along the height axis.
  glm::dvec3 u = glm::normalize(halfAxes[2]);
  // The distance along the height axis.
  double x = abs(glm::dot(positionToCenter, u));
  // The squared distance to the center of the cylinder.
  double d2 = glm::dot(positionToCenter, positionToCenter);
  // The squared *from* the height axis.
  double y2 = d2 - x * x;

  return (x <= 0.5 * this->_height) && (y2 <= r2);
}

BoundingCylinder
BoundingCylinder::transform(const glm::dmat4& transformation) const noexcept {
  return BoundingCylinder(
      glm::dvec3(transformation * glm::dvec4(this->_center, 1.0)),
      glm::dmat3(transformation) * this->_halfAxes);
}

} // namespace CesiumGeometry
