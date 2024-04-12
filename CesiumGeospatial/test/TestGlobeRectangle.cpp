#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/Math.h"

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("GlobeRectangle::fromDegrees example") {
  //! [fromDegrees]
  CesiumGeospatial::GlobeRectangle rectangle =
      CesiumGeospatial::GlobeRectangle::fromDegrees(0.0, 20.0, 10.0, 30.0);
  //! [fromDegrees]
  (void)rectangle;
}

TEST_CASE("GlobeRectangle::isEmpty") { CHECK(GlobeRectangle::EMPTY.isEmpty()); }

TEST_CASE("GlobeRectangle::equals") {
  GlobeRectangle simple(0.1, 0.2, 0.3, 0.4);

  SECTION("returns true for self") {
    CHECK(GlobeRectangle::equals(simple, simple));
  }

  SECTION("returns true for equal rectangle") {
    GlobeRectangle equalRectangle(0.1, 0.2, 0.3, 0.4);
    CHECK(GlobeRectangle::equals(simple, equalRectangle));
  }

  SECTION("returns false for unequal rectangles") {
    GlobeRectangle unequalWest(0.11, 0.2, 0.3, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalWest));

    GlobeRectangle unequalSouth(0.1, 0.202, 0.3, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalSouth));

    GlobeRectangle unequalEast(0.1, 0.2, 0.300004, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalEast));

    GlobeRectangle unequalNorth(0.1, 0.2, 0.3, 0.5);
    CHECK(!GlobeRectangle::equals(simple, unequalNorth));
  }
}

TEST_CASE("GlobeRectangle::equalsEpsilon") {
  GlobeRectangle simple(0.1, 0.2, 0.3, 0.4);

  SECTION("returns true for self") {
    CHECK(GlobeRectangle::equalsEpsilon(simple, simple, Math::Epsilon6));
  }

  SECTION("returns true for exactly equal rectangle") {
    GlobeRectangle equalRectangle(0.1, 0.2, 0.3, 0.4);
    CHECK(
        GlobeRectangle::equalsEpsilon(simple, equalRectangle, Math::Epsilon6));
  }

  SECTION("returns true for rectangle within epsilon") {
    GlobeRectangle epsilonWest(0.10001, 0.200, 0.3, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonWest, Math::Epsilon3));

    GlobeRectangle epsilonSouth(0.1, 0.2002, 0.3, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonSouth, Math::Epsilon3));

    GlobeRectangle epsilonEast(0.1, 0.2, 0.30003, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonEast, Math::Epsilon3));

    GlobeRectangle epsilonNorth(0.1, 0.2, 0.3, 0.4004);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonNorth, Math::Epsilon3));
  }

  SECTION("returns false for rectangle outside epsilon") {
    GlobeRectangle unequalWest(0.11, 0.2, 0.3, 0.4);
    CHECK(!GlobeRectangle::equalsEpsilon(simple, unequalWest, Math::Epsilon3));

    GlobeRectangle unequalSouth(0.1, 0.202, 0.3, 0.4);
    CHECK(!GlobeRectangle::equalsEpsilon(simple, unequalSouth, Math::Epsilon3));

    GlobeRectangle unequalEast(0.1, 0.2, 0.301, 0.4);
    CHECK(!GlobeRectangle::equalsEpsilon(simple, unequalEast, Math::Epsilon3));

    GlobeRectangle unequalNorth(0.1, 0.2, 0.3, 0.5);
    CHECK(!GlobeRectangle::equalsEpsilon(simple, unequalNorth, Math::Epsilon3));
  }
}

TEST_CASE("GlobeRectangle::computeCenter") {
  GlobeRectangle simple(0.1, 0.2, 0.3, 0.4);
  Cartographic center = simple.computeCenter();
  CHECK(Math::equalsEpsilon(center.longitude, 0.2, 0.0, Math::Epsilon14));
  CHECK(Math::equalsEpsilon(center.latitude, 0.3, 0.0, Math::Epsilon14));

  GlobeRectangle wrapping(3.0, 0.2, -3.1, 0.4);
  center = wrapping.computeCenter();
  double expectedLongitude =
      3.0 + ((Math::OnePi - 3.0) + (-3.1 - -Math::OnePi)) * 0.5;
  CHECK(Math::equalsEpsilon(
      center.longitude,
      expectedLongitude,
      0.0,
      Math::Epsilon14));
  CHECK(Math::equalsEpsilon(center.latitude, 0.3, 0.0, Math::Epsilon14));

  GlobeRectangle wrapping2(3.1, 0.2, -3.0, 0.4);
  center = wrapping2.computeCenter();
  expectedLongitude =
      -3.0 - ((Math::OnePi - 3.1) + (-3.0 - -Math::OnePi)) * 0.5;
  CHECK(Math::equalsEpsilon(
      center.longitude,
      expectedLongitude,
      0.0,
      Math::Epsilon14));
  CHECK(Math::equalsEpsilon(center.latitude, 0.3, 0.0, Math::Epsilon14));
}

TEST_CASE("GlobeRectangle::contains") {
  GlobeRectangle simple(0.1, 0.2, 0.3, 0.4);
  CHECK(simple.contains(Cartographic(0.1, 0.2)));
  CHECK(simple.contains(Cartographic(0.1, 0.4)));
  CHECK(simple.contains(Cartographic(0.3, 0.4)));
  CHECK(simple.contains(Cartographic(0.3, 0.2)));
  CHECK(simple.contains(Cartographic(0.2, 0.3)));
  CHECK(!simple.contains(Cartographic(0.0, 0.2)));

  GlobeRectangle wrapping(3.0, 0.2, -3.1, 0.4);
  CHECK(wrapping.contains(Cartographic(Math::OnePi, 0.2)));
  CHECK(wrapping.contains(Cartographic(-Math::OnePi, 0.2)));
  CHECK(wrapping.contains(Cartographic(3.14, 0.2)));
  CHECK(wrapping.contains(Cartographic(-3.14, 0.2)));
  CHECK(!wrapping.contains(Cartographic(0.0, 0.2)));
}
