#include <CesiumGeospatial/GlobeFlightPath.h>

#include <catch2/catch.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("GlobeFlightPath") {
  glm::dvec3 philadelphiaEcef =
      glm::dvec3(1253264.69280105, -4732469.91065521, 4075112.40412297);
  glm::dvec3 tokyoEcef =
      glm::dvec3(-3960158.65587452, 3352568.87555906, 3697235.23506459);

  SECTION("positions at start and end of flight path are identical to input "
          "coordinates") {
    std::optional<GlobeFlightPath> flightPath =
        GlobeFlightPath::fromEarthCenteredEarthFixedCoordinates(
            philadelphiaEcef,
            tokyoEcef);

    CHECK(flightPath.has_value());
    CHECK(Math::equalsEpsilon(
        flightPath.value().getPosition(0.0),
        philadelphiaEcef,
        Math::Epsilon6));
    CHECK(Math::equalsEpsilon(
        flightPath.value().getPosition(1.0),
        tokyoEcef,
        Math::Epsilon6));
  }

  SECTION("should correctly calculate position at 50% through flight path") {
    std::optional<GlobeFlightPath> flightPath =
        GlobeFlightPath::fromEarthCenteredEarthFixedCoordinates(
            philadelphiaEcef,
            tokyoEcef);

    // Values obtained from the Unreal implementation with all curves disabled.
    glm::dvec3 expectedResult = glm::dvec3(
        -2062499.3622640674,
        -1052346.4221710551,
        5923430.4378960524);

    CHECK(flightPath.has_value());
    CHECK(Math::equalsEpsilon(
        flightPath.value().getPosition(0.5),
        expectedResult,
        Math::Epsilon6));
  }

  SECTION("should correctly interpolate height") {
    double startHeight = 100.0;
    double endHeight = 25.0;

    std::optional<GlobeFlightPath> flightPath =
        GlobeFlightPath::fromLongitudeLatitudeHeight(
            Cartographic(25.0, 100.0, startHeight),
            Cartographic(25.0, 100.0, endHeight));

    CHECK(flightPath.has_value());

    std::optional<Cartographic> position25Percent =
        Ellipsoid::WGS84.cartesianToCartographic(
            flightPath.value().getPosition(0.25));
    std::optional<Cartographic> position50Percent =
        Ellipsoid::WGS84.cartesianToCartographic(
            flightPath.value().getPosition(0.5));
    std::optional<Cartographic> position75Percent =
        Ellipsoid::WGS84.cartesianToCartographic(
            flightPath.value().getPosition(0.75));

    CHECK(position25Percent.has_value());
    CHECK(Math::equalsEpsilon(
        position25Percent.value().height,
        (endHeight - startHeight) * 0.25 + startHeight,
        Math::Epsilon6));
    CHECK(position50Percent.has_value());
    CHECK(Math::equalsEpsilon(
        position50Percent.value().height,
        (endHeight - startHeight) * 0.5 + startHeight,
        Math::Epsilon6));
    CHECK(position75Percent.has_value());
    CHECK(Math::equalsEpsilon(
        position75Percent.value().height,
        (endHeight - startHeight) * 0.75 + startHeight,
        Math::Epsilon6));
  }
}
