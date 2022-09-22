#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("LocalHorizontalCoordinateSystem") {
  Cartographic nullIsland = Cartographic(0.0, 0.0, 0.0);
  glm::dvec3 nullIslandEcef =
      Ellipsoid::WGS84.cartographicToCartesian(nullIsland);
  glm::dvec3 oneMeterEastEcef(0.0, 1.0, 0.0);
  glm::dvec3 oneMeterNorthEcef(0.0, 0.0, 1.0);
  glm::dvec3 oneMeterUpEcef(1.0, 0.0, 0.0);

  SECTION("East-north-up") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::North,
        LocalDirection::Up);

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

  SECTION("North-east-down") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::North,
        LocalDirection::East,
        LocalDirection::Down);

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

  SECTION("Left handed East South Up") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up);

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

  SECTION("Left handed East Up North") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::Up,
        LocalDirection::North);

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

  SECTION("Scale") {
    LocalHorizontalCoordinateSystem lh(
        nullIsland,
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0 / 100.0);

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
}
