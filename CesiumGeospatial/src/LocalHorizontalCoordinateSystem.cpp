#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat3x3.hpp>

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
          scaleToMeters) {}

LocalHorizontalCoordinateSystem::LocalHorizontalCoordinateSystem(
    const glm::dvec3& originEcef,
    LocalDirection xAxisDirection,
    LocalDirection yAxisDirection,
    LocalDirection zAxisDirection,
    double scaleToMeters,
    const Ellipsoid& ellipsoid) {
  // The three axes must be orthogonal
  assert(directionToAxis(xAxisDirection) != directionToAxis(yAxisDirection));
  assert(directionToAxis(xAxisDirection) != directionToAxis(zAxisDirection));
  assert(directionToAxis(yAxisDirection) != directionToAxis(zAxisDirection));

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
