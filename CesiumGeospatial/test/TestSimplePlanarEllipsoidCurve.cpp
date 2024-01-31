#include <CesiumGeospatial/SimplePlanarEllipsoidCurve.h>

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

const glm::dvec3
    philadelphiaEcef(1253264.69280105, -4732469.91065521, 4075112.40412297);
const glm::dvec3
    tokyoEcef(-3960158.65587452, 3352568.87555906, 3697235.23506459);

// Values obtained from the Unreal implementation with all curves disabled.
const glm::dvec3 philadelphiaTokyoMidpointEcef(
    -2062499.3622640674,
    -1052346.4221710551,
    5923430.4378960524);

// Equivalent to philadelphiaEcef in Longitude, Latitude, Height
const Cartographic philadelphiaLlh(
    -1.3119164210487293,
    0.6974930673711344,
    373.64791900173714);
// Equivalent to tokyoEcef
const Cartographic
    tokyoLlh(2.4390907007049445, 0.6222806863437318, 283.242432000711);

TEST_CASE("SimplePlanarEllipsoidCurve::getPosition") {
  SECTION("positions at start and end of curve are identical to input "
          "coordinates") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            tokyoEcef);

    CHECK(curve.has_value());
    CHECK(Math::equalsEpsilon(
        curve.value().getPosition(0.0),
        philadelphiaEcef,
        Math::Epsilon6));
    CHECK(Math::equalsEpsilon(
        curve.value().getPosition(1.0),
        tokyoEcef,
        Math::Epsilon6));
  }

  SECTION("should correctly calculate position at 50% through curve") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            tokyoEcef);

    CHECK(curve.has_value());
    glm::dvec3 actualResult = curve.value().getPosition(0.5);

    CHECK(Math::equalsEpsilon(
        actualResult,
        philadelphiaTokyoMidpointEcef,
        Math::Epsilon6));
  }

  SECTION("midpoint of reverse path should be identical") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            tokyoEcef,
            philadelphiaEcef);

    CHECK(curve.has_value());
    glm::dvec3 actualResult = curve.value().getPosition(0.5);

    CHECK(Math::equalsEpsilon(
        actualResult,
        philadelphiaTokyoMidpointEcef,
        Math::Epsilon6));
  }

  SECTION("curve with same start and end point shouldn't change positions") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            philadelphiaEcef);

    CHECK(curve.has_value());

    // check a whole bunch of points along the curve to make sure it stays the
    // same
    const int steps = 25;
    for (int i = 0; i <= steps; i++) {
      glm::dvec3 result = curve.value().getPosition(i / (double)steps);
      CHECK(Math::equalsEpsilon(result, philadelphiaEcef, Math::Epsilon6));
    }
  }

  SECTION("should correctly interpolate height") {
    double startHeight = 100.0;
    double endHeight = 25.0;

    std::optional<SimplePlanarEllipsoidCurve> flightPath =
        SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight(
            Ellipsoid::WGS84,
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

TEST_CASE(
    "SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates") {
  SECTION("should fail on coordinates (0, 0, 0)") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            glm::dvec3(0, 0, 0));

    CHECK(!curve.has_value());
  }
}

TEST_CASE("SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight") {
  SECTION("should match results of curve created from equivalent ECEF "
          "coordinates") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight(
            Ellipsoid::WGS84,
            philadelphiaLlh,
            tokyoLlh);

    CHECK(curve.has_value());
    CHECK(Math::equalsEpsilon(
        curve.value().getPosition(0.0),
        philadelphiaEcef,
        Math::Epsilon6));
    CHECK(Math::equalsEpsilon(
        curve.value().getPosition(1.0),
        tokyoEcef,
        Math::Epsilon6));
    CHECK(Math::equalsEpsilon(
        curve.value().getPosition(0.5),
        philadelphiaTokyoMidpointEcef,
        Math::Epsilon6));
  }
}
