#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>

namespace CesiumGeometry {

namespace {

OrientedBoundingBox computeBoxFromCylinderRegion(
    const glm::dvec3& translation,
    const glm::dquat& rotation,
    double height,
    const glm::dvec2& radialBounds,
    const glm::dvec2& angularBounds) {
  double innerRadius = radialBounds.x;
  double outerRadius = radialBounds.y;
  double angleMin = angularBounds.x;
  double angleMax = angularBounds.y;

  // The radial and angular bounds must be resolved to the scale of a box.
  //
  // First, compute the min and max points (x, y) on the arc drawn by the given
  // angles. Recall that the angle opens counter-clockwise, such that 0 aligns
  // with the +y axis. The -pi/pi discontinuity happens along -y.
  //
  // In polar coordinates, x = r * cos(a) and y = r * sin(a), but this is
  // complicated by the angle starting along the y-axis. Here,
  // x = r * -sin(a) and y = r * cos(a).

  // When the angle range is "reversed", the region sweeps over the -pi / pi
  // discontinuity line.
  bool angleReversed = angleMax < angleMin;
  int angleMaxSign = int(glm::sign(angleMax));
  int angleMinSign = int(glm::sign(angleMin));

  bool angleCrossesZero = angleReversed ? angleMinSign == angleMaxSign
                                        : angleMinSign != angleMaxSign;
  angleCrossesZero |= angleMinSign == 0 || angleMaxSign == 0;

  double yMin =
      angleReversed ? -1.0 : glm::min(glm::cos(angleMin), glm::cos(angleMax));
  double yMax =
      angleCrossesZero ? 1.0 : glm::max(glm::cos(angleMin), glm::cos(angleMax));

  constexpr double piOverTwo = CesiumUtility::Math::PiOverTwo;
  bool angleCrossesNegativePiOverTwo =
      angleReversed ? angleMin <= -piOverTwo || angleMax >= -piOverTwo
                    : angleMin <= -piOverTwo && -piOverTwo <= angleMax;
  bool angleCrossesPiOverTwo =
      angleReversed ? angleMin <= piOverTwo || angleMax >= piOverTwo
                    : angleMin <= piOverTwo && piOverTwo <= angleMax;

  double xMin =
      angleCrossesPiOverTwo ? -1.0 : glm::min(-sin(angleMin), -sin(angleMax));
  double xMax = angleCrossesNegativePiOverTwo
                    ? 1.0
                    : glm::max(-sin(angleMin), -sin(angleMax));

  glm::dvec2 min(xMin, yMin);
  glm::dvec2 max(xMax, yMax);

  // Then, multiply by each radius to compute the circular cross-section.
  // Finally, add scale and compute an axis-aligned box from the points of the
  // region. The cylinder cross-section is flat, so not all corners are needed.

  double zScale = height * 0.5;
  AxisAlignedBox aab = AxisAlignedBox::fromPositions(
      {glm::dvec3(innerRadius * min, -zScale),
       glm::dvec3(innerRadius * max, zScale),
       glm::dvec3(outerRadius * min, -zScale),
       glm::dvec3(outerRadius * max, zScale)});

  OrientedBoundingBox obb = OrientedBoundingBox::fromAxisAligned(aab);
  glm::dvec3 center = obb.getCenter();

  // If the input transform has a rotation, then it must be applied relative to
  // the reference cylinder's origin (instead of the region's center).
  glm::dmat4 centerCylinder = glm::translate(glm::dmat4(1.0), -center);
  glm::dmat4 transformCylinder =
      CesiumGeometry::Transforms::createTranslationRotationScaleMatrix(
          translation,
          rotation,
          glm::dvec3(1.0));
  glm::dmat4 uncenterCylinder = glm::translate(glm::dmat4(1.0), center);
  glm::dmat4 finalTransform =
      uncenterCylinder * transformCylinder * centerCylinder;

  return obb.transform(finalTransform);
}

} // namespace

BoundingCylinderRegion::BoundingCylinderRegion(
    const glm::dvec3& translation,
    const glm::dquat& rotation,
    double height,
    const glm::dvec2& radialBounds,
    const glm::dvec2& angularBounds) noexcept
    : _translation(translation),
      _rotation(rotation),
      _height(height),
      _radialBounds(radialBounds),
      _angularBounds(angularBounds),
      _box(computeBoxFromCylinderRegion(
          translation,
          rotation,
          height,
          radialBounds,
          angularBounds)) {}

CullingResult
BoundingCylinderRegion::intersectPlane(const Plane& plane) const noexcept {
  return this->_box.intersectPlane(plane);
}

double BoundingCylinderRegion::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  return this->_box.computeDistanceSquaredToPosition(position);
}

bool BoundingCylinderRegion::contains(
    const glm::dvec3& position) const noexcept {
  return this->_box.contains(position);
}

BoundingCylinderRegion BoundingCylinderRegion::transform(
    const glm::dmat4& transformation) const noexcept {
  glm::dmat4 originalTransform =
      CesiumGeometry::Transforms::createTranslationRotationScaleMatrix(
          this->_translation,
          this->_rotation,
          glm::dvec3(1.0));

  glm::dmat4 transform = transformation * originalTransform;

  glm::dvec3 translation(0.0);
  glm::dquat rotation(1.0, 0.0, 0.0, 0.0);
  glm::dvec3 scale(1.0);

  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      transform,
      &translation,
      &rotation,
      &scale);

  // The scale of the cylinder region is meant to be captured by the height and
  // radius properties, but it's possible that the region has been scaled.
  // The extension disallows non-uniform scaling of the cylinder's radii; this
  // just picks the bigger scale value.
  double radiusScale = glm::max(scale.x, scale.y);

  return BoundingCylinderRegion(
      translation,
      rotation,
      this->_height * scale.z,
      this->_radialBounds * radiusScale,
      this->_angularBounds);
}

OrientedBoundingBox
BoundingCylinderRegion::toOrientedBoundingBox() const noexcept {
  return this->_box;
}

} // namespace CesiumGeometry
