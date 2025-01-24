#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h>
#include <Cesium3DTilesContent/TileBoundingVolumes.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <doctest/doctest.h>
#include <glm/geometric.hpp>

#include <optional>

using namespace doctest;
using namespace Cesium3DTiles;
using namespace Cesium3DTilesContent;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("TileBoundingVolumes") {
  SUBCASE("box") {
    BoundingVolume bv{};

    // Example bounding box from the 3D Tiles spec
    // clang-format off
    bv.box = {
      0.0,   0.0,   10.0,
      100.0, 0.0,   0.0,
      0.0,   100.0, 0.0,
      0.0,   0.0,   10.0};
    // clang-format on

    std::optional<OrientedBoundingBox> box =
        TileBoundingVolumes::getOrientedBoundingBox(bv);
    REQUIRE(box);

    CHECK(box->getCenter().x == Approx(0.0));
    CHECK(box->getCenter().y == Approx(0.0));
    CHECK(box->getCenter().z == Approx(10.0));
    CHECK(glm::length(box->getHalfAxes()[0]) == Approx(100.0));
    CHECK(glm::length(box->getHalfAxes()[1]) == Approx(100.0));
    CHECK(glm::length(box->getHalfAxes()[2]) == Approx(10.0));

    BoundingVolume next{};
    TileBoundingVolumes::setOrientedBoundingBox(next, *box);
    CHECK(next.box == bv.box);
  }

  SUBCASE("sphere") {
    BoundingVolume bv{};

    // Example bounding sphere from the 3D Tiles spec
    bv.sphere = {0.0, 0.0, 10.0, 141.4214};

    std::optional<BoundingSphere> sphere =
        TileBoundingVolumes::getBoundingSphere(bv);
    REQUIRE(sphere);

    CHECK(sphere->getCenter().x == Approx(0.0));
    CHECK(sphere->getCenter().y == Approx(0.0));
    CHECK(sphere->getCenter().z == Approx(10.0));
    CHECK(sphere->getRadius() == Approx(141.4214));

    BoundingVolume next{};
    TileBoundingVolumes::setBoundingSphere(next, *sphere);
    CHECK(next.sphere == bv.sphere);
  }

  SUBCASE("region") {
    BoundingVolume bv{};

    // Example bounding region from the 3D Tiles spec
    bv.region = {
        -1.3197004795898053,
        0.6988582109,
        -1.3196595204101946,
        0.6988897891,
        0.0,
        20.0};

    std::optional<BoundingRegion> region =
        TileBoundingVolumes::getBoundingRegion(bv, Ellipsoid::WGS84);
    REQUIRE(region);

    CHECK(region->getRectangle().getWest() == Approx(-1.3197004795898053));
    CHECK(region->getRectangle().getSouth() == Approx(0.6988582109));
    CHECK(region->getRectangle().getEast() == Approx(-1.3196595204101946));
    CHECK(region->getRectangle().getNorth() == Approx(0.6988897891));
    CHECK(region->getMinimumHeight() == Approx(0.0));
    CHECK(region->getMaximumHeight() == Approx(20.0));

    BoundingVolume next{};
    TileBoundingVolumes::setBoundingRegion(next, *region);
    CHECK(next.region == bv.region);
  }

  SUBCASE("S2") {
    BoundingVolume bv{};

    // Example from 3DTILES_bounding_volume_S2 spec
    Extension3dTilesBoundingVolumeS2& extension =
        bv.addExtension<Extension3dTilesBoundingVolumeS2>();
    extension.token = "89c6c7";
    extension.minimumHeight = 0.0;
    extension.maximumHeight = 1000.0;

    std::optional<S2CellBoundingVolume> s2 =
        TileBoundingVolumes::getS2CellBoundingVolume(bv, Ellipsoid::WGS84);
    REQUIRE(s2);

    CHECK(s2->getCellID().getID() == S2CellID::fromToken("89c6c7").getID());
    CHECK(s2->getMinimumHeight() == Approx(0.0));
    CHECK(s2->getMaximumHeight() == Approx(1000.0));

    BoundingVolume next{};
    TileBoundingVolumes::setS2CellBoundingVolume(next, *s2);
    Extension3dTilesBoundingVolumeS2* pNextExtension =
        bv.getExtension<Extension3dTilesBoundingVolumeS2>();
    CHECK(pNextExtension->token == extension.token);
    CHECK(pNextExtension->minimumHeight == extension.minimumHeight);
    CHECK(pNextExtension->maximumHeight == extension.maximumHeight);
  }

  SUBCASE("invalid") {
    BoundingVolume bv{};
    CHECK(!TileBoundingVolumes::getOrientedBoundingBox(bv).has_value());
    CHECK(!TileBoundingVolumes::getBoundingSphere(bv).has_value());
    CHECK(!TileBoundingVolumes::getBoundingRegion(bv, Ellipsoid::WGS84)
               .has_value());
    CHECK(!TileBoundingVolumes::getS2CellBoundingVolume(bv, Ellipsoid::WGS84)
               .has_value());
  }
}
