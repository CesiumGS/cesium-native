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
