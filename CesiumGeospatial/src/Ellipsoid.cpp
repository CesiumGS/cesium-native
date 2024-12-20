#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>
#include <optional>

using namespace CesiumUtility;

namespace CesiumGeospatial {

const Ellipsoid Ellipsoid::WGS84(6378137.0, 6378137.0, 6356752.3142451793);
const Ellipsoid Ellipsoid::UNIT_SPHERE(1.0, 1.0, 1.0);

glm::dvec3
Ellipsoid::geodeticSurfaceNormal(const glm::dvec3& position) const noexcept {
  return glm::normalize(position * this->_oneOverRadiiSquared);
}

glm::dvec3 Ellipsoid::geodeticSurfaceNormal(
    const Cartographic& cartographic) const noexcept {
  const double longitude = cartographic.longitude;
  const double latitude = cartographic.latitude;
  const double cosLatitude = glm::cos(latitude);

  return glm::normalize(glm::dvec3(
      cosLatitude * glm::cos(longitude),
      cosLatitude * glm::sin(longitude),
      glm::sin(latitude)));
}

glm::dvec3 Ellipsoid::cartographicToCartesian(
    const Cartographic& cartographic) const noexcept {
  glm::dvec3 n = this->geodeticSurfaceNormal(cartographic);
  glm::dvec3 k = this->_radiiSquared * n;
  const double gamma = sqrt(glm::dot(n, k));
  k /= gamma;
  n *= cartographic.height;
  return k + n;
}

std::optional<Cartographic>
Ellipsoid::cartesianToCartographic(const glm::dvec3& cartesian) const noexcept {
  std::optional<glm::dvec3> p = this->scaleToGeodeticSurface(cartesian);

  if (!p) {
    return std::optional<Cartographic>();
  }

  const glm::dvec3 n = this->geodeticSurfaceNormal(p.value());
  const glm::dvec3 h = cartesian - p.value();

  const double longitude = glm::atan(n.y, n.x);
  const double latitude = glm::asin(n.z);
  const double height = Math::sign(glm::dot(h, cartesian)) * glm::length(h);

  return Cartographic(longitude, latitude, height);
}

std::optional<glm::dvec3>
Ellipsoid::scaleToGeodeticSurface(const glm::dvec3& cartesian) const noexcept {
  const double positionX = cartesian.x;
  const double positionY = cartesian.y;
  const double positionZ = cartesian.z;

  const double oneOverRadiiX = this->_oneOverRadii.x;
  const double oneOverRadiiY = this->_oneOverRadii.y;
  const double oneOverRadiiZ = this->_oneOverRadii.z;

  const double x2 = positionX * positionX * oneOverRadiiX * oneOverRadiiX;
  const double y2 = positionY * positionY * oneOverRadiiY * oneOverRadiiY;
  const double z2 = positionZ * positionZ * oneOverRadiiZ * oneOverRadiiZ;

  // Compute the squared ellipsoid norm.
  const double squaredNorm = x2 + y2 + z2;
  const double ratio = sqrt(1.0 / squaredNorm);

  // As an initial approximation, assume that the radial intersection is the
  // projection point.
  glm::dvec3 intersection = cartesian * ratio;

  // If the position is near the center, the iteration will not converge.
  if (squaredNorm < this->_centerToleranceSquared) {
    return !std::isfinite(ratio) ? std::optional<glm::dvec3>() : intersection;
  }

  const double oneOverRadiiSquaredX = this->_oneOverRadiiSquared.x;
  const double oneOverRadiiSquaredY = this->_oneOverRadiiSquared.y;
  const double oneOverRadiiSquaredZ = this->_oneOverRadiiSquared.z;

  // Use the gradient at the intersection point in place of the true unit
  // normal. The difference in magnitude will be absorbed in the multiplier.
  const glm::dvec3 gradient(
      intersection.x * oneOverRadiiSquaredX * 2.0,
      intersection.y * oneOverRadiiSquaredY * 2.0,
      intersection.z * oneOverRadiiSquaredZ * 2.0);

  // Compute the initial guess at the normal vector multiplier, lambda.
  double lambda =
      ((1.0 - ratio) * glm::length(cartesian)) / (0.5 * glm::length(gradient));
  double correction = 0.0;

  double func;
  double denominator;
  double xMultiplier;
  double yMultiplier;
  double zMultiplier;
  double xMultiplier2;
  double yMultiplier2;
  double zMultiplier2;
  double xMultiplier3;
  double yMultiplier3;
  double zMultiplier3;

  do {
    lambda -= correction;

    xMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredX);
    yMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredY);
    zMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredZ);

    xMultiplier2 = xMultiplier * xMultiplier;
    yMultiplier2 = yMultiplier * yMultiplier;
    zMultiplier2 = zMultiplier * zMultiplier;

    xMultiplier3 = xMultiplier2 * xMultiplier;
    yMultiplier3 = yMultiplier2 * yMultiplier;
    zMultiplier3 = zMultiplier2 * zMultiplier;

    func = x2 * xMultiplier2 + y2 * yMultiplier2 + z2 * zMultiplier2 - 1.0;

    // "denominator" here refers to the use of this expression in the velocity
    // and acceleration computations in the sections to follow.
    denominator = x2 * xMultiplier3 * oneOverRadiiSquaredX +
                  y2 * yMultiplier3 * oneOverRadiiSquaredY +
                  z2 * zMultiplier3 * oneOverRadiiSquaredZ;

    const double derivative = -2.0 * denominator;

    correction = func / derivative;
  } while (glm::abs(func) > Math::Epsilon12);

  return glm::dvec3(
      positionX * xMultiplier,
      positionY * yMultiplier,
      positionZ * zMultiplier);
}

std::optional<glm::dvec3> Ellipsoid::scaleToGeocentricSurface(
    const glm::dvec3& cartesian) const noexcept {

  // If the input cartesian is (0, 0, 0), beta will compute to 1.0 / sqrt(0) =
  // +Infinity. Let's consider this a failure.
  if (Math::equalsEpsilon(glm::length(cartesian), 0, Math::Epsilon12)) {
    return std::optional<glm::dvec3>();
  }

  const double positionX = cartesian.x;
  const double positionY = cartesian.y;
  const double positionZ = cartesian.z;

  const double oneOverRadiiSquaredX = this->_oneOverRadiiSquared.x;
  const double oneOverRadiiSquaredY = this->_oneOverRadiiSquared.y;
  const double oneOverRadiiSquaredZ = this->_oneOverRadiiSquared.z;

  const double beta = 1.0 / sqrt(
                                positionX * positionX * oneOverRadiiSquaredX +
                                positionY * positionY * oneOverRadiiSquaredY +
                                positionZ * positionZ * oneOverRadiiSquaredZ);

  return glm::dvec3(positionX * beta, positionY * beta, positionZ * beta);
}

} // namespace CesiumGeospatial
