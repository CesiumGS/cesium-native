#include <CesiumGeometry/CullingVolume.h>
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