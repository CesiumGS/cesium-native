#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumUtility/Math.h>

#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

using namespace CesiumUtility;

namespace CesiumGeospatial {

/*static*/ glm::dmat4x4 GlobeTransforms::eastNorthUpToFixedFrame(
    const glm::dvec3& origin,
    const Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/) noexcept {
  if (Math::equalsEpsilon(origin, glm::dvec3(0.0), Math::Epsilon14)) {
    // If x, y, and z are zero, use the degenerate local frame, which is a
    // special case
    return glm::dmat4x4(
        glm::dvec4(0.0, 1.0, 0.0, 0.0),
        glm::dvec4(-1.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 1.0, 0.0),
        glm::dvec4(origin, 1.0));
  }
  if (Math::equalsEpsilon(origin.x, 0.0, Math::Epsilon14) &&
      Math::equalsEpsilon(origin.y, 0.0, Math::Epsilon14)) {
    // If x and y are zero, assume origin is at a pole, which is a special case.
    const double sign = Math::sign(origin.z);
    return glm::dmat4x4(
        glm::dvec4(0.0, 1.0, 0.0, 0.0),
        glm::dvec4(-1.0 * sign, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 1.0 * sign, 0.0),
        glm::dvec4(origin, 1.0));
  }

  const glm::dvec3 up = ellipsoid.geodeticSurfaceNormal(origin);
  const glm::dvec3 east = glm::normalize(glm::dvec3(-origin.y, origin.x, 0.0));
  const glm::dvec3 north = glm::cross(up, east);

  return glm::dmat4x4(
      glm::dvec4(east, 0.0),
      glm::dvec4(north, 0.0),
      glm::dvec4(up, 0.0),
      glm::dvec4(origin, 1.0));
}

} // namespace CesiumGeospatial
