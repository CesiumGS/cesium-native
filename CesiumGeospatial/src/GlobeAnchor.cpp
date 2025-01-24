#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeAnchor.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>

#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace {

glm::dmat4 adjustOrientationForMove(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dvec3& oldPosition,
    const glm::dvec3& newPosition,
    const glm::dmat4& anchorToFixed) {
  if (oldPosition == newPosition)
    return anchorToFixed;

  glm::dvec3 oldNormal = ellipsoid.geodeticSurfaceNormal(oldPosition);
  glm::dvec3 newNormal = ellipsoid.geodeticSurfaceNormal(newPosition);

  glm::dmat3 ellipsoidNormalRotation =
      glm::mat3_cast(glm::rotation(oldNormal, newNormal));
  glm::dmat3 newRotationScale =
      ellipsoidNormalRotation * glm::dmat3(anchorToFixed);

  return glm::dmat4(
      glm::dvec4(newRotationScale[0], 0.0),
      glm::dvec4(newRotationScale[1], 0.0),
      glm::dvec4(newRotationScale[2], 0.0),
      anchorToFixed[3]);
}

} // namespace

namespace CesiumGeospatial {

GlobeAnchor GlobeAnchor::fromAnchorToLocalTransform(
    const LocalHorizontalCoordinateSystem& localCoordinateSystem,
    const glm::dmat4& anchorToLocal) {
  return fromAnchorToFixedTransform(
      localCoordinateSystem.getLocalToEcefTransformation() * anchorToLocal);
}

GlobeAnchor
GlobeAnchor::fromAnchorToFixedTransform(const glm::dmat4& anchorToFixed) {
  return GlobeAnchor(anchorToFixed);
}

GlobeAnchor::GlobeAnchor(const glm::dmat4& anchorToFixed)
    : _anchorToFixed(anchorToFixed) {}

const glm::dmat4& GlobeAnchor::getAnchorToFixedTransform() const {
  return this->_anchorToFixed;
}

void GlobeAnchor::setAnchorToFixedTransform(
    const glm::dmat4& newAnchorToFixed,
    bool adjustOrientation,
    const Ellipsoid& ellipsoid) {
  if (adjustOrientation) {
    glm::dvec3 oldPosition = glm::dvec3(this->_anchorToFixed[3]);
    glm::dvec3 newPosition = glm::dvec3(newAnchorToFixed[3]);
    this->_anchorToFixed = adjustOrientationForMove(
        ellipsoid,
        oldPosition,
        newPosition,
        newAnchorToFixed);
  } else {
    this->_anchorToFixed = newAnchorToFixed;
  }
}

glm::dmat4 GlobeAnchor::getAnchorToLocalTransform(
    const LocalHorizontalCoordinateSystem& localCoordinateSystem) const {
  return localCoordinateSystem.getEcefToLocalTransformation() *
         this->_anchorToFixed;
}

void GlobeAnchor::setAnchorToLocalTransform(
    const LocalHorizontalCoordinateSystem& localCoordinateSystem,
    const glm::dmat4& newAnchorToLocal,
    bool adjustOrientation,
    const Ellipsoid& ellipsoid) {
  this->setAnchorToFixedTransform(
      localCoordinateSystem.getLocalToEcefTransformation() * newAnchorToLocal,
      adjustOrientation,
      ellipsoid);
}

} // namespace CesiumGeospatial
