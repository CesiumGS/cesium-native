#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Plane.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

namespace CesiumGeometry {
CullingResult
OrientedBoundingBox::intersectPlane(const Plane& plane) const noexcept {
  const glm::dvec3 normal = plane.getNormal();

  const glm::dmat3& halfAxes = this->getHalfAxes();
  const glm::dvec3& xAxisDirectionAndHalfLength = halfAxes[0];
  const glm::dvec3& yAxisDirectionAndHalfLength = halfAxes[1];
  const glm::dvec3& zAxisDirectionAndHalfLength = halfAxes[2];

  // plane is used as if it is its normal; the first three components are
  // assumed to be normalized
  const double radEffective = glm::abs(
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

  const double distanceToPlane =
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
  const glm::dvec3 offset = position - this->getCenter();

  const glm::dmat3& halfAxes = this->getHalfAxes();
  glm::dvec3 u = halfAxes[0];
  glm::dvec3 v = halfAxes[1];
  glm::dvec3 w = halfAxes[2];

  const double uHalf = glm::length(u);
  const double vHalf = glm::length(v);
  const double wHalf = glm::length(w);

  u /= uHalf;
  v /= vHalf;
  w /= wHalf;

  const glm::dvec3 pPrime(
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

bool OrientedBoundingBox::contains(const glm::dvec3& position) const noexcept {
  glm::dvec3 localPosition = position - this->_center;
  localPosition = this->_inverseHalfAxes * localPosition;
  return glm::abs(localPosition.x) <= 1.0 && glm::abs(localPosition.y) <= 1.0 &&
         glm::abs(localPosition.z) <= 1.0;
}

OrientedBoundingBox OrientedBoundingBox::transform(
    const glm::dmat4& transformation) const noexcept {
  return OrientedBoundingBox(
      glm::dvec3(transformation * glm::dvec4(this->_center, 1.0)),
      glm::dmat3(transformation) * this->_halfAxes);
}

AxisAlignedBox OrientedBoundingBox::toAxisAligned() const noexcept {
  const glm::dmat3& halfAxes = this->_halfAxes;
  glm::dvec3 extent =
      glm::abs(halfAxes[0]) + glm::abs(halfAxes[1]) + glm::abs(halfAxes[2]);
  const glm::dvec3& center = this->_center;
  glm::dvec3 ll = center - extent;
  glm::dvec3 ur = center + extent;
  return AxisAlignedBox(ll.x, ll.y, ll.z, ur.x, ur.y, ur.z);
}

BoundingSphere OrientedBoundingBox::toSphere() const noexcept {
  const glm::dmat3& halfAxes = this->_halfAxes;
  glm::dvec3 corner = halfAxes[0] + halfAxes[1] + halfAxes[2];
  double sphereRadius = glm::length(corner);
  return BoundingSphere(this->_center, sphereRadius);
}

/*static*/ OrientedBoundingBox OrientedBoundingBox::fromAxisAligned(
    const AxisAlignedBox& axisAligned) noexcept {
  return OrientedBoundingBox(
      axisAligned.center,
      glm::dmat3(
          glm::dvec3(axisAligned.lengthX * 0.5, 0.0, 0.0),
          glm::dvec3(0.0, axisAligned.lengthY * 0.5, 0.0),
          glm::dvec3(0.0, 0.0, axisAligned.lengthZ * 0.5)));
}

/*static*/ OrientedBoundingBox
OrientedBoundingBox::fromSphere(const BoundingSphere& sphere) noexcept {
  glm::dvec3 center = sphere.getCenter();
  glm::dmat3 halfAxes = glm::dmat3(sphere.getRadius());
  return OrientedBoundingBox(center, halfAxes);
}

} // namespace CesiumGeometry
