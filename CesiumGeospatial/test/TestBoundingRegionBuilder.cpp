#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <cstring>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("BoundingRegionBuilder::expandToIncludePosition") {
  BoundingRegionBuilder builder;
  GlobeRectangle rectangle = builder.toRegion(Ellipsoid::WGS84).getRectangle();
  CHECK(!rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

  // The rectangle returned by toGlobeRectangle should be identical.
  GlobeRectangle rectangle2 = builder.toGlobeRectangle();
  CHECK(GlobeRectangle::equals(rectangle, rectangle2));

  builder.expandToIncludePosition(Cartographic(0.0, 0.0, 0.0));
  rectangle = builder.toRegion(Ellipsoid::WGS84).getRectangle();
  CHECK(rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

  rectangle2 = builder.toGlobeRectangle();
  CHECK(GlobeRectangle::equals(rectangle, rectangle2));

  builder.expandToIncludePosition(Cartographic(Math::OnePi, 1.0, 0.0));
  rectangle = builder.toRegion(Ellipsoid::WGS84).getRectangle();
  CHECK(rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
  CHECK(rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
  CHECK(rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
  CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

  rectangle2 = builder.toGlobeRectangle();
  CHECK(GlobeRectangle::equals(rectangle, rectangle2));

  BoundingRegionBuilder simpleBuilder = builder;
  simpleBuilder.expandToIncludePosition(Cartographic(-1.0, 1.0, 0.0));
  GlobeRectangle simple =
      simpleBuilder.toRegion(Ellipsoid::WGS84).getRectangle();
  CHECK(simple.contains(Cartographic(0.0, 0.0, 0.0)));
  CHECK(simple.contains(Cartographic(1.0, 0.0, 0.0)));
  CHECK(simple.contains(Cartographic(-1.0, 0.0, 0.0)));
  CHECK(!simple.contains(Cartographic(-3.0, 0.0, 0.0)));
  CHECK(simple.contains(Cartographic(0.0, 1.0, 0.0)));
  CHECK(!simple.contains(Cartographic(0.0, -1.0, 0.0)));

  rectangle2 = simpleBuilder.toGlobeRectangle();
  CHECK(GlobeRectangle::equals(simple, rectangle2));

  BoundingRegionBuilder wrappedBuilder = builder;
  wrappedBuilder.expandToIncludePosition(Cartographic(-3.0, 1.0, 0.0));
  GlobeRectangle wrapped =
      wrappedBuilder.toRegion(Ellipsoid::WGS84).getRectangle();
  CHECK(wrapped.contains(Cartographic(0.0, 0.0, 0.0)));
  CHECK(wrapped.contains(Cartographic(1.0, 0.0, 0.0)));
  CHECK(!wrapped.contains(Cartographic(-1.0, 0.0, 0.0)));
  CHECK(wrapped.contains(Cartographic(-3.0, 0.0, 0.0)));
  CHECK(wrapped.contains(Cartographic(0.0, 1.0, 0.0)));
  CHECK(!wrapped.contains(Cartographic(0.0, -1.0, 0.0)));

  rectangle2 = wrappedBuilder.toGlobeRectangle();
  CHECK(GlobeRectangle::equals(wrapped, rectangle2));
}

TEST_CASE("BoundingRegionBuilder::expandToIncludeGlobeRectangle") {
  SUBCASE("works with simple rectangle as first expand") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(GlobeRectangle(0.1, 0.2, 0.3, 0.4));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle(0.1, 0.2, 0.3, 0.4),
        Math::Epsilon15));

    SUBCASE("does nothing if the rectangle is already included") {
      builder.expandToIncludeGlobeRectangle(
          GlobeRectangle(0.15, 0.25, 0.25, 0.35));
      rectangle = builder.toGlobeRectangle();
      CHECK(GlobeRectangle::equalsEpsilon(
          rectangle,
          GlobeRectangle(0.1, 0.2, 0.3, 0.4),
          Math::Epsilon15));
    }
  }

  SUBCASE("works with anti-meridian crossing rectangle as first expand") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(175.0, -10.0, 173.0, 20.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(175.0, -10.0, 173.0, 20.0),
        Math::Epsilon15));

    SUBCASE("does nothing if the rectangle is already included") {
      builder.expandToIncludeGlobeRectangle(
          GlobeRectangle::fromDegrees(176.0, -9.0, 172.0, 19.0));
      rectangle = builder.toGlobeRectangle();
      CHECK(GlobeRectangle::equalsEpsilon(
          rectangle,
          GlobeRectangle::fromDegrees(175.0, -10.0, 173.0, 20.0),
          Math::Epsilon15));
    }
  }

  SUBCASE("expands simple region across anti-meridian from the west") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(170.0, -10.0, 175.0, 20.0));
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(176.0, -10.0, -175.0, 20.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(170.0, -10.0, -175.0, 20.0),
        Math::Epsilon15));
  }

  SUBCASE("expands simple region across anti-meridian from the east") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(-175.0, -10.0, -170.0, 20.0));
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(175.0, -10.0, -176.0, 20.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(175.0, -10.0, -170.0, 20.0),
        Math::Epsilon15));
  }

  SUBCASE("expands anti-meridian cross region to the west with a simple "
          "rectangle") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(175.0, -10.0, -170.0, 20.0));
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(165.0, -20.0, 170.0, 30.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(165.0, -20.0, -170.0, 30.0),
        Math::Epsilon15));
  }

  SUBCASE("expands anti-meridian cross region to the east with a simple "
          "rectangle") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(175.0, -10.0, -170.0, 20.0));
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(-165.0, -20.0, -160.0, 30.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(175.0, -20.0, -160.0, 30.0),
        Math::Epsilon15));
  }

  SUBCASE("expands anti-meridian crossing rectangle with another one") {
    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(175.0, -10.0, -170.0, 20.0));
    builder.expandToIncludeGlobeRectangle(
        GlobeRectangle::fromDegrees(170.0, -20.0, -160.0, 30.0));
    GlobeRectangle rectangle = builder.toGlobeRectangle();
    CHECK(GlobeRectangle::equalsEpsilon(
        rectangle,
        GlobeRectangle::fromDegrees(170.0, -20.0, -160.0, 30.0),
        Math::Epsilon15));
  }
}
