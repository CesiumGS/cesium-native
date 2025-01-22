#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/SimplePlanarEllipsoidCurve.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <optional>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

const glm::dvec3
    philadelphiaEcef(1253264.69280105, -4732469.91065521, 4075112.40412297);
const glm::dvec3
    tokyoEcef(-3960158.65587452, 3352568.87555906, 3697235.23506459);
// The antipodal position from the philadelphia coordinates above
const glm::dvec3 philadelphiaAntipodeEcef =
    glm::dvec3(-1253369.920224856, 4732412.7444064, -4075146.2160252854);

// Equivalent to philadelphiaEcef in Longitude, Latitude, Height
const Cartographic philadelphiaLlh(
    -1.3119164210487293,
    0.6974930673711344,
    373.64791900173714);
// Equivalent to tokyoEcef
const Cartographic
    tokyoLlh(2.4390907007049445, 0.6222806863437318, 283.242432000711);

// A point above New York City
const glm::dvec3
    newYorkCityEcef(1329752.6826922249, -4657851.870887691, 4140135.1399898543);

// Times Square in NYC
// This point is -10 meters below the surface of the ellipsoid (negative height)
const glm::dvec3
    timesSquareEcef(1334771.9227395034, -4650343.070699833, 4142168.965635141);

TEST_CASE("SimplePlanarEllipsoidCurve::getPosition") {
  SUBCASE("positions at start and end of curve are identical to input "
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

  SUBCASE("all points should be coplanar") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            tokyoEcef);

    CHECK(curve.has_value());
    // needs three points to form a plane - get midpoint to make third point
    glm::dvec3 midpoint = curve.value().getPosition(0.5);
    glm::dvec3 planeNormal = glm::normalize(
        glm::cross(philadelphiaEcef - midpoint, tokyoEcef - midpoint));

    int steps = 100;

    for (int i = 0; i <= steps; i++) {
      double time = 1.0 / (double)steps;
      double dot =
          glm::abs(glm::dot(curve.value().getPosition(time), planeNormal));
      // If the dot product of the point and normal are 0, they're coplanar
      CHECK(Math::equalsEpsilon(dot, 0, Math::Epsilon5));
    }
  }

  SUBCASE("should always stay above the earth") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            philadelphiaAntipodeEcef);

    CHECK(curve.has_value());

    int steps = 100;
    for (int i = 0; i <= steps; i++) {
      double time = 1.0 / (double)steps;
      glm::dvec3 actualResult = curve.value().getPosition(time);

      Cartographic cartographicResult =
          Ellipsoid::WGS84.cartesianToCartographic(actualResult)
              .value_or(Cartographic(0, 0, 0));

      CHECK(cartographicResult.height > 0);
    }
  }

  SUBCASE("midpoint of reverse path should be identical") {
    std::optional<SimplePlanarEllipsoidCurve> forwardCurve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            tokyoEcef);

    CHECK(forwardCurve.has_value());
    glm::dvec3 forwardResult = forwardCurve.value().getPosition(0.5);

    std::optional<SimplePlanarEllipsoidCurve> reverseCurve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            tokyoEcef,
            philadelphiaEcef);

    CHECK(reverseCurve.has_value());
    glm::dvec3 reverseResult = reverseCurve.value().getPosition(0.5);

    CHECK(Math::equalsEpsilon(forwardResult, reverseResult, Math::Epsilon6));
  }

  SUBCASE("curve with same start and end point shouldn't change positions") {
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

  SUBCASE("should correctly interpolate height") {
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

  // Testing a bug in SimplePlanarEllipsoidCurve where a path from a point with
  // negative height to one with positive height would give results on the other
  // side of the earth
  SUBCASE("should correctly handle points with negative height") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            timesSquareEcef,
            newYorkCityEcef);

    CHECK(curve.has_value());
    const double expectedDistance =
        glm::distance(timesSquareEcef, newYorkCityEcef);
    const glm::dvec3 midpoint = curve->getPosition(0.5);
    const double totalActualDistance =
        glm::distance(timesSquareEcef, midpoint) +
        glm::distance(newYorkCityEcef, midpoint);

    CHECK(Math::equalsEpsilon(
        expectedDistance,
        totalActualDistance,
        Math::Epsilon4));
  }
}

TEST_CASE(
    "SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates") {
  SUBCASE("should fail on coordinates (0, 0, 0)") {
    std::optional<SimplePlanarEllipsoidCurve> curve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            glm::dvec3(0, 0, 0));

    CHECK(!curve.has_value());
  }
}

TEST_CASE("SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight") {
  SUBCASE("should match results of curve created from equivalent ECEF "
          "coordinates") {
    std::optional<SimplePlanarEllipsoidCurve> llhCurve =
        SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight(
            Ellipsoid::WGS84,
            philadelphiaLlh,
            tokyoLlh);

    std::optional<SimplePlanarEllipsoidCurve> ecefCurve =
        SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
            Ellipsoid::WGS84,
            philadelphiaEcef,
            tokyoEcef);

    CHECK(llhCurve.has_value());
    CHECK(ecefCurve.has_value());

    int steps = 100;

    for (int i = 0; i <= steps; i++) {
      double n = 1.0 / (double)steps;
      CHECK(Math::equalsEpsilon(
          ecefCurve.value().getPosition(n),
          llhCurve.value().getPosition(n),
          Math::Epsilon6));
    }
  }
}
