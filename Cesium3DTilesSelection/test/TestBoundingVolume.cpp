#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Math.h>

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

  SUBCASE("for BoundingCylinderRegion, a tightly-fitted box is returned") {
    glm::dquat rotation(CesiumGeometry::Transforms::X_UP_TO_Y_UP);
    glm::dvec3 translation(1.0, 2.0, 3.0);

    BoundingCylinderRegion region(
        translation,
        rotation,
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));

    BoundingVolume bv = region;
    OrientedBoundingBox newObb = getOrientedBoundingBoxFromBoundingVolume(bv);

    glm::dvec3 expectedCenter(0.0, 3.0, 3.0);
    glm::dmat3 expectedHalfAxes(0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(CesiumUtility::Math::equalsEpsilon(
        newObb.getCenter(),
        expectedCenter,
        CesiumUtility::Math::Epsilon6));

    const glm::dmat3& halfAxes = newObb.getHalfAxes();
    for (glm::length_t i = 0; i < 3; i++) {
      CHECK(CesiumUtility::Math::equalsEpsilon(
          halfAxes[i],
          expectedHalfAxes[i],
          CesiumUtility::Math::Epsilon6));
    }
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
