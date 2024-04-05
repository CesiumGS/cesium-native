#include "CesiumGeospatial/BoundingRegionBuilder.h"

#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("BoundingRegionBuilder") {
  SECTION("expandToIncludePosition") {
    BoundingRegionBuilder builder;
    GlobeRectangle rectangle = builder.toRegion().getRectangle();
    CHECK(!rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

    builder.expandToIncludePosition(Cartographic(0.0, 0.0, 0.0));
    rectangle = builder.toRegion().getRectangle();
    CHECK(rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

    builder.expandToIncludePosition(Cartographic(Math::OnePi, 1.0, 0.0));
    rectangle = builder.toRegion().getRectangle();
    CHECK(rectangle.contains(Cartographic(0.0, 0.0, 0.0)));
    CHECK(rectangle.contains(Cartographic(1.0, 0.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(-1.0, 0.0, 0.0)));
    CHECK(rectangle.contains(Cartographic(0.0, 1.0, 0.0)));
    CHECK(!rectangle.contains(Cartographic(0.0, -1.0, 0.0)));

    BoundingRegionBuilder simpleBuilder = builder;
    simpleBuilder.expandToIncludePosition(Cartographic(-1.0, 1.0, 0.0));
    GlobeRectangle simple = simpleBuilder.toRegion().getRectangle();
    CHECK(simple.contains(Cartographic(0.0, 0.0, 0.0)));
    CHECK(simple.contains(Cartographic(1.0, 0.0, 0.0)));
    CHECK(simple.contains(Cartographic(-1.0, 0.0, 0.0)));
    CHECK(!simple.contains(Cartographic(-3.0, 0.0, 0.0)));
    CHECK(simple.contains(Cartographic(0.0, 1.0, 0.0)));
    CHECK(!simple.contains(Cartographic(0.0, -1.0, 0.0)));

    BoundingRegionBuilder wrappedBuilder = builder;
    wrappedBuilder.expandToIncludePosition(Cartographic(-3.0, 1.0, 0.0));
    GlobeRectangle wrapped = wrappedBuilder.toRegion().getRectangle();
    CHECK(wrapped.contains(Cartographic(0.0, 0.0, 0.0)));
    CHECK(wrapped.contains(Cartographic(1.0, 0.0, 0.0)));
    CHECK(!wrapped.contains(Cartographic(-1.0, 0.0, 0.0)));
    CHECK(wrapped.contains(Cartographic(-3.0, 0.0, 0.0)));
    CHECK(wrapped.contains(Cartographic(0.0, 1.0, 0.0)));
    CHECK(!wrapped.contains(Cartographic(0.0, -1.0, 0.0)));
  }
}
