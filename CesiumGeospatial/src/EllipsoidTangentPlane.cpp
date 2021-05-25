#include "CesiumGeospatial/EllipsoidTangentPlane.h"
#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"
#include "CesiumGeospatial/Transforms.h"

#include <stdexcept>

using namespace CesiumGeometry;

namespace CesiumGeospatial {

EllipsoidTangentPlane::EllipsoidTangentPlane(
    const glm::dvec3& origin,
    const Ellipsoid& ellipsoid)
    : EllipsoidTangentPlane(
          computeEastNorthUpToFixedFrameUnchecked(origin, ellipsoid),
          ellipsoid) {}

EllipsoidTangentPlane::EllipsoidTangentPlane(
    const glm::dmat4& eastNorthUpToFixedFrame,
    const Ellipsoid& ellipsoid)
    : _ellipsoid(ellipsoid),
      _origin(eastNorthUpToFixedFrame[3]),
      _xAxis(eastNorthUpToFixedFrame[0]),
      _yAxis(eastNorthUpToFixedFrame[1]) 
{
  const glm::dvec3& point = eastNorthUpToFixedFrame[3];
  const glm::dvec3& normal = eastNorthUpToFixedFrame[2];
  double distance = -glm::dot(normal, point);
  _plane = Plane::createUnchecked(normal, distance);
}

glm::dvec2 EllipsoidTangentPlane::projectPointToNearestOnPlane(
    const glm::dvec3& cartesian) const noexcept {
  Ray ray = Ray::createUnchecked(cartesian, this->_plane.getNormal());

  std::optional<glm::dvec3> intersectionPoint =
      IntersectionTests::rayPlane(ray, this->_plane);
  if (!intersectionPoint) {
    intersectionPoint = IntersectionTests::rayPlane(-ray, this->_plane);
    if (!intersectionPoint) {
      intersectionPoint = cartesian;
    }
  }

  glm::dvec3 v = intersectionPoint.value() - this->_origin;
  return glm::dvec2(glm::dot(this->_xAxis, v), glm::dot(this->_yAxis, v));
}

/* static */ glm::dmat4 EllipsoidTangentPlane::computeEastNorthUpToFixedFrameUnchecked(
    const glm::dvec3& origin,
    const Ellipsoid& ellipsoid) {
  const auto scaledOrigin = ellipsoid.scaleToGeodeticSurface(origin);
  if (!scaledOrigin) {
    return glm::dmat4(1.0);
  }
  return Transforms::eastNorthUpToFixedFrame(scaledOrigin.value(), ellipsoid);
}

} // namespace CesiumGeospatial
