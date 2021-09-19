#include "CesiumGeometry/OrientedBoundingBox.h"

#include "CesiumGeometry/Plane.h"

#include <CesiumUtility/Math.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <stdexcept>

namespace CesiumGeometry {
CullingResult
OrientedBoundingBox::intersectPlane(const Plane& plane) const noexcept {
  glm::dvec3 normal = plane.getNormal();

  const glm::dmat3& halfAxes = this->getHalfAxes();
  const glm::dvec3& xAxisDirectionAndHalfLength = halfAxes[0];
  const glm::dvec3& yAxisDirectionAndHalfLength = halfAxes[1];
  const glm::dvec3& zAxisDirectionAndHalfLength = halfAxes[2];

  // plane is used as if it is its normal; the first three components are
  // assumed to be normalized
  double radEffective = glm::abs(
                            normal.x * xAxisDirectionAndHalfLength.x +
                            normal.y * xAxisDirectionAndHalfLength.y +
                            normal.z * xAxisDirectionAndHalfLength.z) +
                        glm::abs(
                            normal.x * yAxisDirectionAndHalfLength.x +
                            normal.y * yAxisDirectionAndHalfLength.y +
                            normal.z * yAxisDirectionAndHalfLength.z) +
                        glm::abs(
                            normal.x * zAxisDirectionAndHalfLength.x +
                            normal.y * zAxisDirectionAndHalfLength.y +
                            normal.z * zAxisDirectionAndHalfLength.z);

  double distanceToPlane =
      ::glm::dot(normal, this->getCenter()) + plane.getDistance();

  if (distanceToPlane <= -radEffective) {
    // The entire box is on the negative side of the plane normal
    return CullingResult::Outside;
  }
  if (distanceToPlane >= radEffective) {
    // The entire box is on the positive side of the plane normal
    return CullingResult::Inside;
  }
  return CullingResult::Intersecting;
}

double OrientedBoundingBox::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  glm::dvec3 offset = position - this->getCenter();

  const glm::dmat3& halfAxes = this->getHalfAxes();
  glm::dvec3 u = halfAxes[0];
  glm::dvec3 v = halfAxes[1];
  glm::dvec3 w = halfAxes[2];

  double uHalf = glm::length(u);
  double vHalf = glm::length(v);
  double wHalf = glm::length(w);

  u /= uHalf;
  v /= vHalf;
  w /= wHalf;

  glm::dvec3 pPrime(
      glm::dot(offset, u),
      glm::dot(offset, v),
      glm::dot(offset, w));

  double distanceSquared = 0.0;
  double d;

  if (pPrime.x < -uHalf) {
    d = pPrime.x + uHalf;
    distanceSquared += d * d;
  } else if (pPrime.x > uHalf) {
    d = pPrime.x - uHalf;
    distanceSquared += d * d;
  }

  if (pPrime.y < -vHalf) {
    d = pPrime.y + vHalf;
    distanceSquared += d * d;
  } else if (pPrime.y > vHalf) {
    d = pPrime.y - vHalf;
    distanceSquared += d * d;
  }

  if (pPrime.z < -wHalf) {
    d = pPrime.z + wHalf;
    distanceSquared += d * d;
  } else if (pPrime.z > wHalf) {
    d = pPrime.z - wHalf;
    distanceSquared += d * d;
  }

  return distanceSquared;
}

} // namespace CesiumGeometry
