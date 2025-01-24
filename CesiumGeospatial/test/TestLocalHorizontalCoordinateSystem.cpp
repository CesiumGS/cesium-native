#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("LocalHorizontalCoordinateSystem") {
  Cartographic nullIsland = Cartographic(0.0, 0.0, 0.0);
  glm::dvec3 nullIslandEcef =
      Ellipsoid::WGS84.cartographicToCartesian(nullIsland);
  glm::dvec3 oneMeterEastEcef(0.0, 1.0, 0.0);
  glm::dvec3 oneMeterNorthEcef(0.0, 0.0, 1.0);
  glm::dvec3 oneMeterUpEcef(1.0, 0.0, 0.0);

  SUBCASE("East-north-up") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::North,
        LocalDirection::Up,
        1.0,
        Ellipsoid::WGS84);

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterEastEcef),
        glm::dvec3(1.0, 0.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterNorthEcef),
        glm::dvec3(0.0, 1.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterUpEcef),
        glm::dvec3(0.0, 0.0, 1.0),
        0.0,
        1e-10));
  }

  SUBCASE("North-east-down") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::North,
        LocalDirection::East,
        LocalDirection::Down,
        1.0,
        Ellipsoid::WGS84);

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterEastEcef),
        glm::dvec3(0.0, 1.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterNorthEcef),
        glm::dvec3(1.0, 0.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterUpEcef),
        glm::dvec3(0.0, 0.0, -1.0),
        0.0,
        1e-10));
  }

  SUBCASE("Left handed East South Up") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0,
        Ellipsoid::WGS84);

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterEastEcef),
        glm::dvec3(1.0, 0.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterNorthEcef),
        glm::dvec3(0.0, -1.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterUpEcef),
        glm::dvec3(0.0, 0.0, 1.0),
        0.0,
        1e-10));
  }

  SUBCASE("Left handed East Up North") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::Up,
        LocalDirection::North,
        1.0,
        Ellipsoid::WGS84);

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterEastEcef),
        glm::dvec3(1.0, 0.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterNorthEcef),
        glm::dvec3(0.0, 0.0, 1.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterUpEcef),
        glm::dvec3(0.0, 1.0, 0.0),
        0.0,
        1e-10));
  }

  SUBCASE("Scale") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0 / 100.0,
        Ellipsoid::WGS84);

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterEastEcef),
        glm::dvec3(100.0, 0.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterNorthEcef),
        glm::dvec3(0.0, -100.0, 0.0),
        0.0,
        1e-10));

    CHECK(Math::equalsEpsilon(
        lh.ecefPositionToLocal(nullIslandEcef + oneMeterUpEcef),
        glm::dvec3(0.0, 0.0, 100.0),
        0.0,
        1e-10));
  }

  SUBCASE("computeTransformationToAnotherLocal") {
    LocalHorizontalCoordinateSystem original(
        nullIsland,
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0,
        Ellipsoid::WGS84);

    LocalHorizontalCoordinateSystem target(
        Cartographic::fromDegrees(12.0, 23.0, 1000.0),
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0,
        Ellipsoid::WGS84);

    glm::dvec3 somePointInOriginal = glm::dvec3(1781.0, 373.0, 7777.2);
    glm::dvec3 samePointInEcef =
        original.localPositionToEcef(somePointInOriginal);
    glm::dvec3 samePointInTarget = target.ecefPositionToLocal(samePointInEcef);

    glm::dmat4 transform = original.computeTransformationToAnotherLocal(target);
    glm::dvec3 computedByTransform =
        glm::dvec3(transform * glm::dvec4(somePointInOriginal, 1.0));
    CHECK(Math::equalsEpsilon(computedByTransform, samePointInTarget, 1e-15));
  }
}
