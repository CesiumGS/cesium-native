#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>

#include <array>
#include <cstdint>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("S2CellID") {
  SUBCASE("constructor") {
    S2CellID id(3458764513820540928U);
    CHECK(id.isValid());
    CHECK(id.getID() == 3458764513820540928U);
  }

  SUBCASE("creates an invalid token for an invalid ID") {
    S2CellID id(uint64_t(-1));
    CHECK(!id.isValid());
  }

  SUBCASE("creates cell from valid token") {
    S2CellID id = S2CellID::fromToken("3");
    CHECK(id.isValid());
    CHECK(id.getID() == 3458764513820540928U);
  }

  SUBCASE("creates invalid from invalid token") {
    S2CellID id = S2CellID::fromToken("XX");
    CHECK(!id.isValid());
  }

  SUBCASE("accepts valid token") {
    CHECK(S2CellID::fromToken("1").isValid());
    CHECK(S2CellID::fromToken("2ef59bd34").isValid());
    CHECK(S2CellID::fromToken("2ef59bd352b93ac3").isValid());
  }

  SUBCASE("rejects token of invalid value") {
    CHECK(!S2CellID::fromToken("LOL").isValid());
    CHECK(!S2CellID::fromToken("----").isValid());
    CHECK(!S2CellID::fromToken(std::string(17, '9')).isValid());
    CHECK(!S2CellID::fromToken("0").isValid());
    CHECK(!S2CellID::fromToken("🤡").isValid());
  }

  SUBCASE("accepts valid cell ID") {
    CHECK(S2CellID(3383782026967071428U).isValid());
    CHECK(S2CellID(3458764513820540928).isValid());
  }

  SUBCASE("rejects invalid cell ID") {
    CHECK(!S2CellID(0U).isValid());
    CHECK(!S2CellID(uint64_t(-1)).isValid());
    CHECK(
        !S2CellID(
             0b0010101000000000000000000000000000000000000000000000000000000000U)
             .isValid());
  }

  SUBCASE("correctly converts token to cell ID") {
    CHECK(S2CellID::fromToken("04").getID() == 288230376151711744U);
    CHECK(S2CellID::fromToken("3").getID() == 3458764513820540928U);
    CHECK(
        S2CellID::fromToken("2ef59bd352b93ac3").getID() ==
        3383782026967071427U);
  }

  SUBCASE("gets correct level of cell") {
    CHECK(S2CellID(3170534137668829184U).getLevel() == 1);
    CHECK(S2CellID(3383782026921377792U).getLevel() == 16);
    CHECK(S2CellID(3383782026967071427U).getLevel() == 30);
  }

  SUBCASE("gets correct center of cell") {
    Cartographic center = S2CellID::fromToken("1").getCenter();
    CHECK(Math::equalsEpsilon(center.longitude, 0.0, 0.0, Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.latitude, 0.0, 0.0, Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("3").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(90.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.latitude, 0.0, 0.0, Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("5").getCenter();
    // The "longitude" of the south pole is a meaningless question, so the value
    // the implementation returns is arbitrary, and in fact has changed between
    // the prior version and 0.11.0 (the current version).
    // CHECK(Math::equalsEpsilon(
    //     center.longitude,
    //     Math::degreesToRadians(0),
    //     0.0,
    //     Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(90.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("7").getCenter();
    // The "longitude" of the international dateline can either be -180 or 180,
    // depending on the implementation, so we need to take the absolute value.
    CHECK(Math::equalsEpsilon(
        glm::abs(center.longitude),
        Math::degreesToRadians(180.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(0.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("9").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(-90.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(0.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("b").getCenter();
    // Don't validate the "longitude" of the south pole, as it's meaningless.
    // CHECK(Math::equalsEpsilon(
    //     center.longitude,
    //     Math::degreesToRadians(0.0),
    //     0.0,
    //     Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(-90.0),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("2ef59bd352b93ac3").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(105.64131803774308),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(-10.490091033598308),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));

    center = S2CellID::fromToken("1234567").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(9.868307318504081),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(27.468392925827605),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::Epsilon10));
  }

  SUBCASE("gets correct vertices of cell") {
    std::array<Cartographic, 4> vertices =
        S2CellID::fromToken("2ef59bd352b93ac3").getVertices();

    CHECK(Math::equalsEpsilon(
        vertices[0].longitude,
        Math::degreesToRadians(105.64131799299665),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        vertices[0].latitude,
        Math::degreesToRadians(-10.490091077431977),
        0.0,
        Math::Epsilon10));

    CHECK(Math::equalsEpsilon(
        vertices[1].longitude,
        Math::degreesToRadians(105.64131808248949),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        vertices[1].latitude,
        Math::degreesToRadians(-10.490091072946313),
        0.0,
        Math::Epsilon10));

    CHECK(Math::equalsEpsilon(
        vertices[2].longitude,
        Math::degreesToRadians(105.64131808248948),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        vertices[2].latitude,
        Math::degreesToRadians(-10.490090989764633),
        0.0,
        Math::Epsilon10));

    CHECK(Math::equalsEpsilon(
        vertices[3].longitude,
        Math::degreesToRadians(105.64131799299665),
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        vertices[3].latitude,
        Math::degreesToRadians(-10.4900909942503),
        0.0,
        Math::Epsilon10));
  }

  SUBCASE("fromQuadtreeTileID") {
    S2CellID a = S2CellID::fromQuadtreeTileID(
        S2CellID::fromToken("1").getFace(),
        QuadtreeTileID(0, 0, 0));
    CHECK(a.getID() == S2CellID::fromToken("1").getID());
    S2CellID b = S2CellID::fromQuadtreeTileID(
        S2CellID::fromToken("1").getFace(),
        QuadtreeTileID(1, 0, 0));
    CHECK(b.getID() == S2CellID::fromToken("04").getID());
    S2CellID c = S2CellID::fromQuadtreeTileID(
        S2CellID::fromToken("1").getFace(),
        QuadtreeTileID(1, 1, 0));
    CHECK(c.getID() == S2CellID::fromToken("1c").getID());
    S2CellID d = S2CellID::fromQuadtreeTileID(
        S2CellID::fromToken("1").getFace(),
        QuadtreeTileID(1, 0, 1));
    CHECK(d.getID() == S2CellID::fromToken("0c").getID());
    S2CellID e = S2CellID::fromQuadtreeTileID(
        S2CellID::fromToken("1").getFace(),
        QuadtreeTileID(1, 1, 1));
    CHECK(e.getID() == S2CellID::fromToken("14").getID());
  }

  SUBCASE("computeBoundingRectangle") {
    S2CellID root0 = S2CellID::fromFaceLevelPosition(0, 0, 0);
    GlobeRectangle root0Rect = root0.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root0Rect.getWest(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root0Rect.getEast(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root0Rect.getSouth(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root0Rect.getNorth(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));

    S2CellID root1 = S2CellID::fromFaceLevelPosition(1, 0, 0);
    GlobeRectangle root1Rect = root1.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root1Rect.getWest(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root1Rect.getEast(),
        3.0 * Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root1Rect.getSouth(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root1Rect.getNorth(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));

    S2CellID root2 = S2CellID::fromFaceLevelPosition(2, 0, 0);
    GlobeRectangle root2Rect = root2.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root2Rect.getWest(),
        -Math::OnePi,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root2Rect.getEast(),
        Math::OnePi,
        0.0,
        Math::Epsilon14));
    // The midpoint of the cell edge is at 45 degrees latitude, but the vertices
    // extend significantly lower.
    CHECK(root2Rect.getSouth() < Math::OnePi / 4.0 - Math::OnePi / 20.0);
    CHECK(Math::equalsEpsilon(
        root2Rect.getNorth(),
        Math::OnePi / 2.0,
        0.0,
        Math::Epsilon14));

    S2CellID root3 = S2CellID::fromFaceLevelPosition(3, 0, 0);
    GlobeRectangle root3Rect = root3.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root3Rect.getWest(),
        3.0 * Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root3Rect.getEast(),
        -3.0 * Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root3Rect.getSouth(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root3Rect.getNorth(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));

    S2CellID root4 = S2CellID::fromFaceLevelPosition(4, 0, 0);
    GlobeRectangle root4Rect = root4.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root4Rect.getWest(),
        -3.0 * Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root4Rect.getEast(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root4Rect.getSouth(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root4Rect.getNorth(),
        Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));

    S2CellID root5 = S2CellID::fromFaceLevelPosition(5, 0, 0);
    GlobeRectangle root5Rect = root5.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        root5Rect.getWest(),
        -Math::OnePi,
        0.0,
        Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        root5Rect.getEast(),
        Math::OnePi,
        0.0,
        Math::Epsilon14));
    // The midpoint of the cell edge is at -45 degrees latitude, but the
    // vertices extend significantly higher.
    CHECK(Math::equalsEpsilon(
        root5Rect.getSouth(),
        -Math::OnePi / 2.0,
        0.0,
        Math::Epsilon14));
    CHECK(root5Rect.getNorth() > -Math::OnePi / 4.0 + Math::OnePi / 20.0);

    S2CellID equatorCell = S2CellID::fromFaceLevelPosition(0, 1, 0);
    GlobeRectangle equatorRect = equatorCell.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(
        equatorRect.getWest(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(
        Math::equalsEpsilon(equatorRect.getEast(), 0.0, 0.0, Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        equatorRect.getSouth(),
        -Math::OnePi / 4.0,
        0.0,
        Math::Epsilon14));
    CHECK(
        Math::equalsEpsilon(equatorRect.getNorth(), 0.0, 0.0, Math::Epsilon14));

    S2CellID polarCell = S2CellID::fromFaceLevelPosition(2, 1, 0);
    GlobeRectangle polarRect = polarCell.computeBoundingRectangle();
    CHECK(Math::equalsEpsilon(polarRect.getWest(), 0.0, 0.0, Math::Epsilon14));
    CHECK(Math::equalsEpsilon(
        polarRect.getEast(),
        Math::OnePi / 2.0,
        0.0,
        Math::Epsilon14));
    // One vertex of the cell at 45 degrees latitude, but the other extends
    // significantly lower.
    CHECK(root2Rect.getSouth() < Math::OnePi / 4.0 - Math::OnePi / 20.0);
    CHECK(Math::equalsEpsilon(
        polarRect.getNorth(),
        Math::OnePi / 2,
        0.0,
        Math::Epsilon14));
  }
}
