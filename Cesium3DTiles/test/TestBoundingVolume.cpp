#include <Cesium3DTiles/BoundingVolume.h>
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
#include <CesiumGltf/Extension3dTilesShapeCylinderRegion.h>
#include <CesiumGltf/Extension3dTilesShapeEllipsoidRegion.h>
#include <CesiumGltf/Extension3dTilesShapeS2.h>
#include <CesiumGltf/Shape.h>
#include <CesiumTilesetJson/BoundingVolume.h>
#include <CesiumTilesetJson/Extension3dTilesBoundingVolumeCylinder.h>
#include <CesiumTilesetJson/Extension3dTilesBoundingVolumeS2.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <numbers>

using namespace doctest;
using namespace Cesium3DTiles;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("getBoundingVolume") {
  SUBCASE("from 3D Tiles 1.1") {
    SUBCASE("box") {
      CesiumTilesetJson::BoundingVolume bv;
      // clang-format off
      bv.box = {
        0.0,   0.0,   10.0,
        100.0, 0.0,   0.0,
        0.0,   100.0, 0.0,
        0.0,   0.0,   10.0};
      // clang-format on
      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(bv, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pBox =
          std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
      REQUIRE(pBox);
      CHECK_EQ(pBox->getCenter(), glm::dvec3(0.0, 0.0, 10.0));
      CHECK_EQ(
          pBox->getHalfAxes(),
          glm::dmat3(100.0, 0.0, 0.0, 0.0, 100.0, 0.0, 0.0, 0.0, 10.0));
    }

    SUBCASE("sphere") {
      CesiumTilesetJson::BoundingVolume bv;
      bv.sphere = {2.0, -3.0, 10.0, 17.0};

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(bv, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pSphere =
          std::get_if<CesiumGeometry::BoundingSphere>(&boundingVolume);
      REQUIRE(pSphere);
      CHECK_EQ(pSphere->getCenter(), glm::dvec3(2.0, -3.0, 10.0));
      CHECK_EQ(pSphere->getRadius(), 17.0);
    }

    SUBCASE("region") {
      CesiumTilesetJson::BoundingVolume bv;
      bv.region = {
          -1.3197004795898053,
          0.6988582109,
          -1.3196595204101946,
          0.6988897891,
          0.0,
          20.0};

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(bv, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pRegion =
          std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
      REQUIRE(pRegion);
      CHECK_EQ(pRegion->getRectangle().getWest(), Approx(-1.3197004795898053));
      CHECK_EQ(pRegion->getRectangle().getSouth(), Approx(0.6988582109));
      CHECK_EQ(pRegion->getRectangle().getEast(), Approx(-1.3196595204101946));
      CHECK_EQ(pRegion->getRectangle().getNorth(), Approx(0.6988897891));
      CHECK_EQ(pRegion->getMinimumHeight(), Approx(0.0));
      CHECK_EQ(pRegion->getMaximumHeight(), Approx(20.0));
    }

    SUBCASE("S2") {
      CesiumTilesetJson::BoundingVolume bv;
      auto& extension = bv.addExtension<
          CesiumTilesetJson::Extension3dTilesBoundingVolumeS2>();
      extension.token = "89c6c7";
      extension.minimumHeight = 0.0;
      extension.maximumHeight = 1000.0;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(bv, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pS2 =
          std::get_if<CesiumGeospatial::S2CellBoundingVolume>(&boundingVolume);
      CHECK_EQ(pS2->getCellID().getID(), S2CellID::fromToken("89c6c7").getID());
      CHECK_EQ(pS2->getMinimumHeight(), Approx(0.0));
      CHECK_EQ(pS2->getMaximumHeight(), Approx(1000.0));
    }

    SUBCASE("cylinder") {
      CesiumTilesetJson::BoundingVolume bv;
      auto& extension = bv.addExtension<
          CesiumTilesetJson::Extension3dTilesBoundingVolumeCylinder>();
      extension.translation = {1.0, 2.0, 3.0};
      extension.rotation = {0.0, 0.0, 0.0, 1.0};
      extension.height = 2.0;
      extension.minRadius = 1.0;
      extension.maxRadius = 2.0;
      extension.minAngle = -Math::PiOverTwo;
      extension.maxAngle = Math::PiOverTwo;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(bv, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pCylinder =
          std::get_if<CesiumGeometry::BoundingCylinderRegion>(&boundingVolume);
      CHECK_EQ(pCylinder->getTranslation(), glm::dvec3(1.0, 2.0, 3.0));
      CHECK_EQ(pCylinder->getRotation(), glm::dquat(1.0, 0.0, 0.0, 0.0));
      CHECK_EQ(pCylinder->getHeight(), 2.0);
      CHECK_EQ(pCylinder->getRadialBounds(), glm::dvec2(1.0, 2.0));
      CHECK_EQ(
          pCylinder->getAngularBounds(),
          glm::dvec2(-Math::PiOverTwo, Math::PiOverTwo));
    }
  }

  SUBCASE("from glTF") {
    SUBCASE("box") {
      CesiumGltf::Shape shape;
      shape.box.emplace().size = {1.0, 2.0, 3.0};

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(shape, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pBox =
          std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
      REQUIRE(pBox);
      CHECK_EQ(pBox->getCenter(), glm::dvec3(0.0));
      CHECK_EQ(
          pBox->getHalfAxes(),
          glm::dmat3(1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 3.0));
    }

    SUBCASE("sphere") {
      CesiumGltf::Shape shape;
      shape.sphere.emplace().radius = 5.5;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(shape, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pSphere =
          std::get_if<CesiumGeometry::BoundingSphere>(&boundingVolume);
      REQUIRE(pSphere);
      CHECK_EQ(pSphere->getCenter(), glm::dvec3(0.0));
      CHECK_EQ(pSphere->getRadius(), 5.5);
    }

    SUBCASE("region") {
      CesiumGltf::Shape shape;
      auto& extension =
          shape
              .addExtension<CesiumGltf::Extension3dTilesShapeEllipsoidRegion>();
      extension.minimumLatitude = 0.6988582109;
      extension.maximumLatitude = 0.6988897891;
      extension.minimumLongitude = -1.3197004795898053;
      extension.maximumLongitude = -1.3196595204101946;
      extension.minimumHeight = 0.0;
      extension.maximumHeight = 20.0;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(shape, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pRegion =
          std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
      REQUIRE(pRegion);
      CHECK_EQ(pRegion->getRectangle().getWest(), Approx(-1.3197004795898053));
      CHECK_EQ(pRegion->getRectangle().getSouth(), Approx(0.6988582109));
      CHECK_EQ(pRegion->getRectangle().getEast(), Approx(-1.3196595204101946));
      CHECK_EQ(pRegion->getRectangle().getNorth(), Approx(0.6988897891));
      CHECK_EQ(pRegion->getMinimumHeight(), Approx(0.0));
      CHECK_EQ(pRegion->getMaximumHeight(), Approx(20.0));
    }

    SUBCASE("S2") {
      CesiumGltf::Shape shape;
      auto& extension =
          shape.addExtension<CesiumGltf::Extension3dTilesShapeS2>();
      extension.token = "89c6c7";
      extension.minimumHeight = 0.0;
      extension.maximumHeight = 1000.0;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(shape, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pS2 =
          std::get_if<CesiumGeospatial::S2CellBoundingVolume>(&boundingVolume);
      CHECK_EQ(pS2->getCellID().getID(), S2CellID::fromToken("89c6c7").getID());
      CHECK_EQ(pS2->getMinimumHeight(), Approx(0.0));
      CHECK_EQ(pS2->getMaximumHeight(), Approx(1000.0));
    }

    SUBCASE("cylinder") {
      CesiumGltf::Shape shape;
      auto& extension =
          shape.addExtension<CesiumGltf::Extension3dTilesShapeCylinderRegion>();
      extension.height = 2.0;
      extension.minimumRadius = 1.0;
      extension.maximumRadius = 2.0;
      extension.minimumAngle = -Math::PiOverTwo;
      extension.maximumAngle = Math::PiOverTwo;

      Result<Cesium3DTiles::BoundingVolume> result =
          getBoundingVolume(shape, CesiumGeospatial::Ellipsoid::WGS84);
      REQUIRE_UNARY(result.value);

      Cesium3DTiles::BoundingVolume& boundingVolume = *result.value;
      auto* pCylinder =
          std::get_if<CesiumGeometry::BoundingCylinderRegion>(&boundingVolume);
      CHECK_EQ(pCylinder->getTranslation(), glm::dvec3(0.0));
      CHECK_EQ(pCylinder->getRotation(), glm::dquat(1.0, 0.0, 0.0, 0.0));
      CHECK_EQ(pCylinder->getHeight(), 2.0);
      CHECK_EQ(pCylinder->getRadialBounds(), glm::dvec2(1.0, 2.0));
      CHECK_EQ(
          pCylinder->getAngularBounds(),
          glm::dvec2(-Math::PiOverTwo, Math::PiOverTwo));
    }
  }
}

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

TEST_CASE("testIntersection") {
  SUBCASE("OrientedBoundingBox") {
    // Simple tests with oriented bounding boxes
    // Two boxes rotated 45 degrees around Z in opposite directions
    using namespace std::numbers;
    glm::dmat3 counterclockwise(
        {sqrt2 / 2.0, sqrt2 / 2.0, 0.0},
        {-sqrt2 / 2.0, sqrt2 / 2.0, 0.0},
        {0.0, 0.0, 1.0});
    glm::dmat3 clockwise(
        {sqrt2 / 2.0, -sqrt2 / 2.0, 0.0},
        {sqrt2 / 2.0, sqrt2 / 2.0, 0.0},
        {0.0, 0.0, 1.0});
    OrientedBoundingBox obb0({-2.0, 0.0, 0.0}, counterclockwise);
    OrientedBoundingBox obb1({2.0, 0.0, 0.0}, clockwise);
    CHECK(!testIntersection(obb0, obb1));
    OrientedBoundingBox obb2({-1.0, 0.0, 0.0}, counterclockwise);
    OrientedBoundingBox obb3({1.0, 0.0, 0.0}, clockwise);
    CHECK(testIntersection(obb2, obb3));
  }
  SUBCASE("BoundingRegions") {
    // A "nautical mile" square in Philadelphia
    BoundingRegion phl(
        GlobeRectangle(
            -1.3120159199172432,
            0.6969344194233807,
            -1.311257041597562,
            0.6975161958407122),
        0.0,
        300.0);
    // A nautical mile square in NYC
    BoundingRegion nyc(
        GlobeRectangle(
            -1.2921574402846652,
            0.710289268896309,
            -1.2913899085981675,
            0.7108710453136404),
        0.0,
        300.0);
    CHECK(!testIntersection(phl, nyc));
  }
}
