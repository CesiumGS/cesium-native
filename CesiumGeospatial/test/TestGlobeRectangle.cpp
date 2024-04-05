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

TEST_CASE("GlobeRectangle") {
  SECTION("isEmpty") { CHECK(GlobeRectangle::EMPTY.isEmpty()); }

  SECTION("computeCenter") {
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

  SECTION("contains") {
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

  SECTION("splitAtAntiMeridian") {
    GlobeRectangle nonCrossing =
        GlobeRectangle::fromDegrees(-10.0, -20.0, 30.0, 40.0);
    std::pair<GlobeRectangle, std::optional<GlobeRectangle>> split =
        nonCrossing.splitAtAntiMeridian();
    CHECK(!split.second);
    CHECK(split.first.getWest() == nonCrossing.getWest());
    CHECK(split.first.getEast() == nonCrossing.getEast());
    CHECK(split.first.getSouth() == nonCrossing.getSouth());
    CHECK(split.first.getNorth() == nonCrossing.getNorth());

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
  }
}
