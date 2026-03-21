#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumQuantizedMeshTerrain/Layer.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <optional>
#include <variant>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumQuantizedMeshTerrain;
using namespace CesiumUtility;

TEST_CASE("LayerJsonUtilities") {
  SUBCASE("getProjection") {
    Layer layer;
    std::optional<Projection> maybeProjection;

    layer.projection = "EPSG:4326";
    maybeProjection = layer.getProjection(Ellipsoid::WGS84);
    REQUIRE(maybeProjection);
    CHECK(std::get_if<GeographicProjection>(&*maybeProjection) != nullptr);

    layer.projection = "EPSG:3857";
    maybeProjection = layer.getProjection(Ellipsoid::WGS84);
    REQUIRE(maybeProjection);
    CHECK(std::get_if<WebMercatorProjection>(&*maybeProjection) != nullptr);

    layer.projection = "foo";
    maybeProjection = layer.getProjection(Ellipsoid::WGS84);
    CHECK(!maybeProjection);
  }

  SUBCASE("getTilingScheme") {
    Layer layer;
    std::optional<QuadtreeTilingScheme> maybeTilingScheme;

    layer.projection = "EPSG:4326";
    maybeTilingScheme = layer.getTilingScheme(Ellipsoid::WGS84);
    REQUIRE(maybeTilingScheme);
    CHECK(maybeTilingScheme->getRootTilesX() == 2);
    CHECK(maybeTilingScheme->getRootTilesY() == 1);
    CHECK(
        maybeTilingScheme->getRectangle().getLowerLeft() ==
        GeographicProjection::computeMaximumProjectedRectangle(Ellipsoid::WGS84)
            .getLowerLeft());
    CHECK(
        maybeTilingScheme->getRectangle().getUpperRight() ==
        GeographicProjection::computeMaximumProjectedRectangle(Ellipsoid::WGS84)
            .getUpperRight());

    layer.projection = "EPSG:3857";
    maybeTilingScheme = layer.getTilingScheme(Ellipsoid::WGS84);
    REQUIRE(maybeTilingScheme);
    CHECK(maybeTilingScheme->getRootTilesX() == 1);
    CHECK(maybeTilingScheme->getRootTilesY() == 1);
    CHECK(Math::equalsEpsilon(
        maybeTilingScheme->getRectangle().getLowerLeft(),
        WebMercatorProjection::computeMaximumProjectedRectangle(
            Ellipsoid::WGS84)
            .getLowerLeft(),
        1e-14));
    CHECK(Math::equalsEpsilon(
        maybeTilingScheme->getRectangle().getUpperRight(),
        WebMercatorProjection::computeMaximumProjectedRectangle(
            Ellipsoid::WGS84)
            .getUpperRight(),
        1e-14));

    layer.projection = "foo";
    maybeTilingScheme = layer.getTilingScheme(Ellipsoid::WGS84);
    CHECK(!maybeTilingScheme);
  }

  SUBCASE("getRootBoundingRegion") {
    Layer layer;
    std::optional<BoundingRegion> maybeBoundingRegion;

    layer.projection = "EPSG:4326";
    maybeBoundingRegion = layer.getRootBoundingRegion(Ellipsoid::WGS84);
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
    maybeBoundingRegion = layer.getRootBoundingRegion(Ellipsoid::WGS84);
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
    maybeBoundingRegion = layer.getRootBoundingRegion(Ellipsoid::WGS84);
    CHECK(!maybeBoundingRegion);
  }
}
