#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>

#include <doctest/doctest.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("getOrientedBoundingBoxFromBoundingVolume") {
  SUBCASE("for OrientedBoundingBox, the box is returned directly") {
    OrientedBoundingBox obb(
        glm::dvec3(1.0, 2.0, 3.0),
        glm::dmat3(
            glm::dvec3(1.0, 2.0, 3.0),
            glm::dvec3(4.0, 5.0, 6.0),
            glm::dvec3(7.0, 8.0, 9.0)));
    BoundingVolume bv = obb;
    OrientedBoundingBox newObb = getOrientedBoundingBoxFromBoundingVolume(bv);
    CHECK(obb.getCenter() == newObb.getCenter());
    CHECK(obb.getHalfAxes() == newObb.getHalfAxes());
  }

  SUBCASE("for BoundingSphere, a circumscribed box is returned") {
    BoundingSphere bs(glm::dvec3(1.0, 2.0, 3.0), 10.0);
    BoundingVolume bv = bs;
    OrientedBoundingBox newObb = getOrientedBoundingBoxFromBoundingVolume(bv);
    CHECK(newObb.getCenter() == bs.getCenter());
    CHECK(newObb.getLengths() == glm::dvec3(20.0));
  }

  SUBCASE("for others, their aggregated oriented bounding box is returned") {
    BoundingRegion region(
        GlobeRectangle(0.5, 1.0, 1.5, 2.0),
        100.0,
        200.0,
        Ellipsoid::WGS84);
    BoundingVolume bv = region;
    OrientedBoundingBox newObb = getOrientedBoundingBoxFromBoundingVolume(bv);
    CHECK(region.getBoundingBox().getCenter() == newObb.getCenter());
    CHECK(region.getBoundingBox().getHalfAxes() == newObb.getHalfAxes());

    BoundingRegionWithLooseFittingHeights looseRegion(region);
    bv = looseRegion;
    newObb = getOrientedBoundingBoxFromBoundingVolume(bv);
    CHECK(region.getBoundingBox().getCenter() == newObb.getCenter());
    CHECK(region.getBoundingBox().getHalfAxes() == newObb.getHalfAxes());

    S2CellBoundingVolume s2(
        S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(10, 1, 2)),
        100.0,
        200.0,
        Ellipsoid::WGS84);
    bv = s2;
    newObb = getOrientedBoundingBoxFromBoundingVolume(bv);
    CHECK(
        s2.computeBoundingRegion().getBoundingBox().getCenter() ==
        newObb.getCenter());
    CHECK(
        s2.computeBoundingRegion().getBoundingBox().getHalfAxes() ==
        newObb.getHalfAxes());
  }
}
