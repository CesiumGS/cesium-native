#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <optional>
#include <utility>

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

  SUBCASE("returns true for self") {
    CHECK(GlobeRectangle::equals(simple, simple));
  }

  SUBCASE("returns true for equal rectangle") {
    GlobeRectangle equalRectangle(0.1, 0.2, 0.3, 0.4);
    CHECK(GlobeRectangle::equals(simple, equalRectangle));
  }

  SUBCASE("returns false for unequal rectangles") {
    GlobeRectangle unequalWest(0.11, 0.2, 0.3, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalWest));

    GlobeRectangle unequalSouth(0.1, 0.202, 0.3, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalSouth));

    GlobeRectangle unequalEast(0.1, 0.2, 0.300004, 0.4);
    CHECK(!GlobeRectangle::equals(simple, unequalEast));

    GlobeRectangle unequalNorth(0.1, 0.2, 0.3, 0.5);
    CHECK(!GlobeRectangle::equals(simple, unequalNorth));
  }

  SUBCASE("splitAtAntiMeridian") {
    // Cross Prime meridian, do not cross Antimeridian
    GlobeRectangle nonCrossing =
        GlobeRectangle::fromDegrees(-10.0, -20.0, 30.0, 40.0);
    std::pair<GlobeRectangle, std::optional<GlobeRectangle>> split =
        nonCrossing.splitAtAntiMeridian();
    CHECK(!split.second);
    CHECK(split.first.getWest() == nonCrossing.getWest());
    CHECK(split.first.getEast() == nonCrossing.getEast());
    CHECK(split.first.getSouth() == nonCrossing.getSouth());
    CHECK(split.first.getNorth() == nonCrossing.getNorth());

    // Do not cross Prime or Antimeridian
    GlobeRectangle nonCrossing2 =
        GlobeRectangle::fromDegrees(10.0, -20.0, 30.0, 40.0);
    split = nonCrossing2.splitAtAntiMeridian();
    CHECK(!split.second);
    CHECK(split.first.getWest() == nonCrossing2.getWest());
    CHECK(split.first.getEast() == nonCrossing2.getEast());
    CHECK(split.first.getSouth() == nonCrossing2.getSouth());
    CHECK(split.first.getNorth() == nonCrossing2.getNorth());

    // Cross Antimeridian
    GlobeRectangle crossing1 =
        GlobeRectangle::fromDegrees(160.0, -20.0, -170.0, 40.0);
    split = crossing1.splitAtAntiMeridian();
    CHECK(split.first.getWest() == crossing1.getWest());
    CHECK(split.first.getEast() == Math::OnePi);
    CHECK(split.first.getSouth() == crossing1.getSouth());
    CHECK(split.first.getNorth() == crossing1.getNorth());
    REQUIRE(split.second);
    CHECK(split.second->getWest() == -Math::OnePi);
    CHECK(split.second->getEast() == crossing1.getEast());
    CHECK(split.second->getSouth() == crossing1.getSouth());
    CHECK(split.second->getNorth() == crossing1.getNorth());

    // Same test, offset, with different returned order
    GlobeRectangle crossing2 =
        GlobeRectangle::fromDegrees(170.0, -20.0, -160.0, 40.0);
    split = crossing2.splitAtAntiMeridian();
    CHECK(split.first.getWest() == -Math::OnePi);
    CHECK(split.first.getEast() == crossing2.getEast());
    CHECK(split.first.getSouth() == crossing2.getSouth());
    CHECK(split.first.getNorth() == crossing2.getNorth());
    REQUIRE(split.second);
    CHECK(split.second->getWest() == crossing2.getWest());
    CHECK(split.second->getEast() == Math::OnePi);
    CHECK(split.second->getSouth() == crossing2.getSouth());
    CHECK(split.second->getNorth() == crossing2.getNorth());

    // Cross Prime and Antimeridian
    GlobeRectangle crossing3 =
        GlobeRectangle::fromDegrees(-10.0, -20.0, -160.0, 40.0);
    split = crossing3.splitAtAntiMeridian();
    CHECK(split.first.getWest() == crossing3.getWest());
    CHECK(split.first.getEast() == Math::OnePi);
    CHECK(split.first.getSouth() == crossing3.getSouth());
    CHECK(split.first.getNorth() == crossing3.getNorth());
    REQUIRE(split.second);
    CHECK(split.second->getWest() == -Math::OnePi);
    CHECK(split.second->getEast() == crossing3.getEast());
    CHECK(split.second->getSouth() == crossing3.getSouth());
    CHECK(split.second->getNorth() == crossing3.getNorth());
  }
}

TEST_CASE("GlobeRectangle::equalsEpsilon") {
  GlobeRectangle simple(0.1, 0.2, 0.3, 0.4);

  SUBCASE("returns true for self") {
    CHECK(GlobeRectangle::equalsEpsilon(simple, simple, Math::Epsilon6));
  }

  SUBCASE("returns true for exactly equal rectangle") {
    GlobeRectangle equalRectangle(0.1, 0.2, 0.3, 0.4);
    CHECK(
        GlobeRectangle::equalsEpsilon(simple, equalRectangle, Math::Epsilon6));
  }

  SUBCASE("returns true for rectangle within epsilon") {
    GlobeRectangle epsilonWest(0.10001, 0.200, 0.3, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonWest, Math::Epsilon3));

    GlobeRectangle epsilonSouth(0.1, 0.2002, 0.3, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonSouth, Math::Epsilon3));

    GlobeRectangle epsilonEast(0.1, 0.2, 0.30003, 0.4);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonEast, Math::Epsilon3));

    GlobeRectangle epsilonNorth(0.1, 0.2, 0.3, 0.4004);
    CHECK(GlobeRectangle::equalsEpsilon(simple, epsilonNorth, Math::Epsilon3));
  }

  SUBCASE("returns false for rectangle outside epsilon") {
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
