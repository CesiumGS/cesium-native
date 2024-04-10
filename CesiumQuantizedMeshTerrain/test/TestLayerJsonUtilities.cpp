#include <CesiumQuantizedMeshTerrain/Layer.h>
#include <CesiumQuantizedMeshTerrain/LayerJsonUtilities.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumQuantizedMeshTerrain;
using namespace CesiumUtility;

TEST_CASE("LayerJsonUtilities") {
  SECTION("getProjection") {
    Layer layer;
    std::optional<Projection> maybeProjection;

    layer.projection = "EPSG:4326";
    maybeProjection = LayerJsonUtilities::getProjection(layer);
    REQUIRE(maybeProjection);
    CHECK(std::get_if<GeographicProjection>(&*maybeProjection) != nullptr);

    layer.projection = "EPSG:3857";
    maybeProjection = LayerJsonUtilities::getProjection(layer);
    REQUIRE(maybeProjection);
    CHECK(std::get_if<WebMercatorProjection>(&*maybeProjection) != nullptr);

    layer.projection = "foo";
    maybeProjection = LayerJsonUtilities::getProjection(layer);
    CHECK(!maybeProjection);
  }

  SECTION("getTilingScheme") {
    Layer layer;
    std::optional<QuadtreeTilingScheme> maybeTilingScheme;

    layer.projection = "EPSG:4326";
    maybeTilingScheme = LayerJsonUtilities::getTilingScheme(layer);
    REQUIRE(maybeTilingScheme);
    CHECK(maybeTilingScheme->getRootTilesX() == 2);
    CHECK(maybeTilingScheme->getRootTilesY() == 1);
    CHECK(
        maybeTilingScheme->getRectangle().getLowerLeft() ==
        GeographicProjection::computeMaximumProjectedRectangle()
            .getLowerLeft());
    CHECK(
        maybeTilingScheme->getRectangle().getUpperRight() ==
        GeographicProjection::computeMaximumProjectedRectangle()
            .getUpperRight());

    layer.projection = "EPSG:3857";
    maybeTilingScheme = LayerJsonUtilities::getTilingScheme(layer);
    REQUIRE(maybeTilingScheme);
    CHECK(maybeTilingScheme->getRootTilesX() == 1);
    CHECK(maybeTilingScheme->getRootTilesY() == 1);
    CHECK(Math::equalsEpsilon(
        maybeTilingScheme->getRectangle().getLowerLeft(),
        WebMercatorProjection::computeMaximumProjectedRectangle()
            .getLowerLeft(),
        1e-14));
    CHECK(Math::equalsEpsilon(
        maybeTilingScheme->getRectangle().getUpperRight(),
        WebMercatorProjection::computeMaximumProjectedRectangle()
            .getUpperRight(),
        1e-14));

    layer.projection = "foo";
    maybeTilingScheme = LayerJsonUtilities::getTilingScheme(layer);
    CHECK(!maybeTilingScheme);
  }

  SECTION("getRootBoundingRegion") {
    Layer layer;
    std::optional<BoundingRegion> maybeBoundingRegion;

    layer.projection = "EPSG:4326";
    maybeBoundingRegion = LayerJsonUtilities::getRootBoundingRegion(layer);
    REQUIRE(maybeBoundingRegion);
    CHECK(
        maybeBoundingRegion->getRectangle().getWest() ==
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE.getWest());
    CHECK(
        maybeBoundingRegion->getRectangle().getSouth() ==
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE.getSouth());
    CHECK(
        maybeBoundingRegion->getRectangle().getEast() ==
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE.getEast());
    CHECK(
        maybeBoundingRegion->getRectangle().getNorth() ==
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE.getNorth());
    CHECK(maybeBoundingRegion->getMinimumHeight() == -1000.0);
    CHECK(maybeBoundingRegion->getMaximumHeight() == 9000.0);

    layer.projection = "EPSG:3857";
    maybeBoundingRegion = LayerJsonUtilities::getRootBoundingRegion(layer);
    REQUIRE(maybeBoundingRegion);
    CHECK(
        maybeBoundingRegion->getRectangle().getWest() ==
        WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE.getWest());
    CHECK(
        maybeBoundingRegion->getRectangle().getSouth() ==
        WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE.getSouth());
    CHECK(
        maybeBoundingRegion->getRectangle().getEast() ==
        WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE.getEast());
    CHECK(
        maybeBoundingRegion->getRectangle().getNorth() ==
        WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE.getNorth());
    CHECK(maybeBoundingRegion->getMinimumHeight() == -1000.0);
    CHECK(maybeBoundingRegion->getMaximumHeight() == 9000.0);

    layer.projection = "foo";
    maybeBoundingRegion = LayerJsonUtilities::getRootBoundingRegion(layer);
    CHECK(!maybeBoundingRegion);
  }
}
