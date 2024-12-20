#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumUtility/Assert.h>

#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using namespace CesiumGeospatial;

namespace {

enum class DirectionAxis { WestEast, SouthNorth, DownUp };

[[maybe_unused]] DirectionAxis directionToAxis(LocalDirection direction) {
  switch (direction) {
  case LocalDirection::West:
  case LocalDirection::East:
    return DirectionAxis::WestEast;
  case LocalDirection::South:
  case LocalDirection::North:
    return DirectionAxis::SouthNorth;
  case LocalDirection::Down:
  case LocalDirection::Up:
  default:
    return DirectionAxis::DownUp;
  }
}

glm::dvec3 directionToEnuVector(LocalDirection direction) {
  switch (direction) {
  case LocalDirection::West:
    return glm::dvec3(-1.0, 0.0, 0.0);
  case LocalDirection::East:
    return glm::dvec3(1.0, 0.0, 0.0);
  case LocalDirection::South:
    return glm::dvec3(0.0, -1.0, 0.0);
  case LocalDirection::North:
    return glm::dvec3(0.0, 1.0, 0.0);
  case LocalDirection::Down:
    return glm::dvec3(0.0, 0.0, -1.0);
  case LocalDirection::Up:
  default:
    return glm::dvec3(0.0, 0.0, 1.0);
  }
}

} // namespace

LocalHorizontalCoordinateSystem::LocalHorizontalCoordinateSystem(
    const Cartographic& origin,
    LocalDirection xAxisDirection,
    LocalDirection yAxisDirection,
    LocalDirection zAxisDirection,
    double scaleToMeters,
    const Ellipsoid& ellipsoid)
    : LocalHorizontalCoordinateSystem(
          ellipsoid.cartographicToCartesian(origin),
          xAxisDirection,
          yAxisDirection,
          zAxisDirection,
          scaleToMeters,
          ellipsoid) {}

LocalHorizontalCoordinateSystem::LocalHorizontalCoordinateSystem(
    const glm::dvec3& originEcef,
    LocalDirection xAxisDirection,
    LocalDirection yAxisDirection,
    LocalDirection zAxisDirection,
    double scaleToMeters,
    const Ellipsoid& ellipsoid) {
  // The three axes must be orthogonal
  CESIUM_ASSERT(
      directionToAxis(xAxisDirection) != directionToAxis(yAxisDirection));
  CESIUM_ASSERT(
      directionToAxis(xAxisDirection) != directionToAxis(zAxisDirection));
  CESIUM_ASSERT(
      directionToAxis(yAxisDirection) != directionToAxis(zAxisDirection));

  // Construct a local horizontal system with East-North-Up axes (right-handed)
  glm::dmat4 enuToFixed =
      GlobeTransforms::eastNorthUpToFixedFrame(originEcef, ellipsoid);

  // Construct a matrix to swap and invert axes as necessary
  glm::dmat3 localToEnuAndScale(
      scaleToMeters * directionToEnuVector(xAxisDirection),
      scaleToMeters * directionToEnuVector(yAxisDirection),
      scaleToMeters * directionToEnuVector(zAxisDirection));

  this->_localToEcef = enuToFixed * glm::dmat4(localToEnuAndScale);
  this->_ecefToLocal = glm::affineInverse(this->_localToEcef);
}

CesiumGeospatial::LocalHorizontalCoordinateSystem::
    LocalHorizontalCoordinateSystem(const glm::dmat4& localToEcef)
    : _ecefToLocal(glm::affineInverse(localToEcef)),
      _localToEcef(localToEcef) {}

CesiumGeospatial::LocalHorizontalCoordinateSystem::
    LocalHorizontalCoordinateSystem(
        const glm::dmat4& localToEcef,
        const glm::dmat4& ecefToLocal)
    : _ecefToLocal(ecefToLocal), _localToEcef(localToEcef) {}

glm::dvec3 LocalHorizontalCoordinateSystem::localPositionToEcef(
    const glm::dvec3& localPosition) const noexcept {
  return glm::dvec3(this->_localToEcef * glm::dvec4(localPosition, 1.0));
}

glm::dvec3 LocalHorizontalCoordinateSystem::ecefPositionToLocal(
    const glm::dvec3& ecefPosition) const noexcept {
  return glm::dvec3(this->_ecefToLocal * glm::dvec4(ecefPosition, 1.0));
}

glm::dvec3 LocalHorizontalCoordinateSystem::localDirectionToEcef(
    const glm::dvec3& localDirection) const noexcept {
  return glm::dvec3(this->_localToEcef * glm::dvec4(localDirection, 0.0));
}

glm::dvec3 LocalHorizontalCoordinateSystem::ecefDirectionToLocal(
    const glm::dvec3& ecefDirection) const noexcept {
  return glm::dvec3(this->_ecefToLocal * glm::dvec4(ecefDirection, 0.0));
}

glm::dmat4 LocalHorizontalCoordinateSystem::computeTransformationToAnotherLocal(
    const LocalHorizontalCoordinateSystem& target) const noexcept {
  return target.getEcefToLocalTransformation() *
         this->getLocalToEcefTransformation();
}
