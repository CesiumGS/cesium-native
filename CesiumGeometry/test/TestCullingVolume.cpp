#include <CesiumGeometry/CullingVolume.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <cmath>

using namespace doctest;
using namespace CesiumGeometry;
using namespace CesiumUtility;

namespace {
bool equalsEpsilon(
    const Plane& left,
    const Plane& right,
    double relativeEpsilon) {
  return Math::equalsEpsilon(
             left.getNormal(),
             right.getNormal(),
             relativeEpsilon) &&
         Math::equalsEpsilon(
             left.getDistance(),
             right.getDistance(),
             relativeEpsilon);
}
} // namespace

bool equalsEpsilon(
    const CullingVolume& left,
    const CullingVolume& right,
    double relativeEpsilon) {
  return equalsEpsilon(left.leftPlane, right.leftPlane, relativeEpsilon) &&
         equalsEpsilon(left.rightPlane, right.rightPlane, relativeEpsilon) &&
         equalsEpsilon(left.topPlane, right.topPlane, relativeEpsilon) &&
         equalsEpsilon(left.bottomPlane, right.bottomPlane, relativeEpsilon);
}

TEST_CASE("CullingVolume::createCullingVolume") {
  SUBCASE("Shouldn't crash when too far from the globe") {
    CHECK_NOTHROW(createCullingVolume(
        glm::dvec3(1e20, 1e20, 1e20),
        glm::dvec3(0, 0, 1),
        glm::dvec3(0, 1, 0),
        Math::PiOverTwo,
        Math::PiOverTwo));
  }

  SUBCASE("Shouldn't crash at the center of the globe") {
    CHECK_NOTHROW(createCullingVolume(
        glm::dvec3(0, 0, 0),
        glm::dvec3(0, 0, 1),
        glm::dvec3(0, 1, 0),
        Math::PiOverTwo,
        Math::PiOverTwo));
  }
}

TEST_CASE("CullingVolume construction") {
  SUBCASE("Field of view and clip matrix") {
    glm::dvec3 position(1e5, 1e5, 1e5);
    glm::dvec3 direction(0, 0, 1);
    glm::dvec3 up(0, 1, 0);
    CullingVolume traditional = createCullingVolume(
        position,
        direction,
        up,
        Math::PiOverTwo,
        Math::PiOverTwo);
    CullingVolume rad = createCullingVolume(
        Transforms::createPerspectiveMatrix(
            Math::PiOverTwo,
            Math::PiOverTwo,
            10,
            200000) *
        Transforms::createViewMatrix(position, direction, up));
    CHECK(equalsEpsilon(traditional, rad, 1e-10));
  }
}
