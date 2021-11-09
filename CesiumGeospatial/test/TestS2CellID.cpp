#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("S2CellID") {
  SECTION("constructor") {
    S2CellID id(3458764513820540928U);
    CHECK(id.isValid());
    CHECK(id.getID() == 3458764513820540928U);
  }

  SECTION("creates an invalid token for an invalid ID") {
    S2CellID id(uint64_t(-1));
    CHECK(!id.isValid());
  }

  SECTION("creates cell from valid token") {
    S2CellID id = S2CellID::fromToken("3");
    CHECK(id.isValid());
    CHECK(id.getID() == 3458764513820540928U);
  }

  SECTION("creates invalid from invalid token") {
    S2CellID id = S2CellID::fromToken("XX");
    CHECK(!id.isValid());
  }

  SECTION("accepts valid token") {
    CHECK(S2CellID::fromToken("1").isValid());
    CHECK(S2CellID::fromToken("2ef59bd34").isValid());
    CHECK(S2CellID::fromToken("2ef59bd352b93ac3").isValid());
  }

  SECTION("rejects token of invalid value") {
    CHECK(!S2CellID::fromToken("LOL").isValid());
    CHECK(!S2CellID::fromToken("----").isValid());
    CHECK(!S2CellID::fromToken(std::string(17, '9')).isValid());
    CHECK(!S2CellID::fromToken("0").isValid());
    CHECK(!S2CellID::fromToken("🤡").isValid());
  }

  SECTION("accepts valid cell ID") {
    CHECK(S2CellID(3383782026967071428U).isValid());
    CHECK(S2CellID(3458764513820540928).isValid());
  }

  SECTION("rejects invalid cell ID") {
    CHECK(!S2CellID(0U).isValid());
    CHECK(!S2CellID(uint64_t(-1)).isValid());
    CHECK(
        !S2CellID(
             0b0010101000000000000000000000000000000000000000000000000000000000U)
             .isValid());
  }

  SECTION("correctly converts token to cell ID") {
    CHECK(S2CellID::fromToken("04").getID() == 288230376151711744U);
    CHECK(S2CellID::fromToken("3").getID() == 3458764513820540928U);
    CHECK(
        S2CellID::fromToken("2ef59bd352b93ac3").getID() ==
        3383782026967071427U);
  }

  SECTION("gets correct level of cell") {
    CHECK(S2CellID(3170534137668829184U).getLevel() == 1);
    CHECK(S2CellID(3383782026921377792U).getLevel() == 16);
    CHECK(S2CellID(3383782026967071427U).getLevel() == 30);
  }

  SECTION("gets correct center of cell") {
    Cartographic center = S2CellID::fromToken("1").getCenter();
    CHECK(Math::equalsEpsilon(center.longitude, 0.0, 0.0, Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.latitude, 0.0, 0.0, Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("3").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(90.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.latitude, 0.0, 0.0, Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("5").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(-180.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(90.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("7").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(-180.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(0.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("9").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(-90.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(0.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("b").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(0.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(-90.0),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("2ef59bd352b93ac3").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(105.64131803774308),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(-10.490091033598308),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));

    center = S2CellID::fromToken("1234567").getCenter();
    CHECK(Math::equalsEpsilon(
        center.longitude,
        Math::degreesToRadians(9.868307318504081),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        center.latitude,
        Math::degreesToRadians(27.468392925827605),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(center.height, 0.0, 0.0, Math::EPSILON10));
  }

  SECTION("gets correct vertices of cell") {
    std::array<Cartographic, 4> vertices =
        S2CellID::fromToken("2ef59bd352b93ac3").getVertices();

    CHECK(Math::equalsEpsilon(
        vertices[0].longitude,
        Math::degreesToRadians(105.64131799299665),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        vertices[0].latitude,
        Math::degreesToRadians(-10.490091077431977),
        0.0,
        Math::EPSILON10));

    CHECK(Math::equalsEpsilon(
        vertices[1].longitude,
        Math::degreesToRadians(105.64131808248949),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        vertices[1].latitude,
        Math::degreesToRadians(-10.490091072946313),
        0.0,
        Math::EPSILON10));

    CHECK(Math::equalsEpsilon(
        vertices[2].longitude,
        Math::degreesToRadians(105.64131808248948),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        vertices[2].latitude,
        Math::degreesToRadians(-10.490090989764633),
        0.0,
        Math::EPSILON10));

    CHECK(Math::equalsEpsilon(
        vertices[3].longitude,
        Math::degreesToRadians(105.64131799299665),
        0.0,
        Math::EPSILON10));
    CHECK(Math::equalsEpsilon(
        vertices[3].latitude,
        Math::degreesToRadians(-10.4900909942503),
        0.0,
        Math::EPSILON10));
  }

  SECTION("fromQuadtreeTileID") {
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
}
