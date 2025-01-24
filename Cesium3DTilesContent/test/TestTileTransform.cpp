#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTilesContent/TileTransform.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>

#include <optional>

using namespace CesiumUtility;

TEST_CASE("TileTransform::getTransform") {
  SUBCASE("correctly interprets a valid transform") {
    Cesium3DTiles::Tile tile;
    tile.transform = {
        1.0,
        2.0,
        3.0,
        4.0,
        5.0,
        6.0,
        7.0,
        8.0,
        9.0,
        10.0,
        11.0,
        12.0,
        13.0,
        14.0,
        15.0,
        16.0};
    std::optional<glm::dmat4> maybeTransform =
        Cesium3DTilesContent::TileTransform::getTransform(tile);
    REQUIRE(maybeTransform);
    const glm::dmat4& transform = *maybeTransform;
    CHECK(Math::equalsEpsilon(
        transform[0],
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[1],
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[2],
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[3],
        glm::dvec4(13.0, 14.0, 15.0, 16.0),
        1e-14));
  }

  SUBCASE("returns nullopt on too few elements") {
    Cesium3DTiles::Tile tile;
    tile.transform = {
        1.0,
        2.0,
        3.0,
        4.0,
        5.0,
        6.0,
        7.0,
        8.0,
        9.0,
        10.0,
        11.0,
        12.0,
        13.0,
        14.0,
        15.0};
    std::optional<glm::dmat4> maybeTransform =
        Cesium3DTilesContent::TileTransform::getTransform(tile);
    REQUIRE(!maybeTransform);
  }

  SUBCASE("ignores extra elements") {
    Cesium3DTiles::Tile tile;
    tile.transform = {
        1.0,
        2.0,
        3.0,
        4.0,
        5.0,
        6.0,
        7.0,
        8.0,
        9.0,
        10.0,
        11.0,
        12.0,
        13.0,
        14.0,
        15.0,
        16.0,
        17.0};
    std::optional<glm::dmat4> maybeTransform =
        Cesium3DTilesContent::TileTransform::getTransform(tile);
    REQUIRE(maybeTransform);
    const glm::dmat4& transform = *maybeTransform;
    CHECK(Math::equalsEpsilon(
        transform[0],
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[1],
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[2],
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        1e-14));
    CHECK(Math::equalsEpsilon(
        transform[3],
        glm::dvec4(13.0, 14.0, 15.0, 16.0),
        1e-14));
  }
}

TEST_CASE("TileTransform::setTransform") {
  SUBCASE("correctly sets the transform") {
    glm::dmat4 transform(
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        glm::dvec4(13.0, 14.0, 15.0, 16.0));

    Cesium3DTiles::Tile tile;
    Cesium3DTilesContent::TileTransform::setTransform(tile, transform);

    REQUIRE(tile.transform.size() == 16);
    CHECK(tile.transform[0] == 1.0);
    CHECK(tile.transform[1] == 2.0);
    CHECK(tile.transform[2] == 3.0);
    CHECK(tile.transform[3] == 4.0);
    CHECK(tile.transform[4] == 5.0);
    CHECK(tile.transform[5] == 6.0);
    CHECK(tile.transform[6] == 7.0);
    CHECK(tile.transform[7] == 8.0);
    CHECK(tile.transform[8] == 9.0);
    CHECK(tile.transform[9] == 10.0);
    CHECK(tile.transform[10] == 11.0);
    CHECK(tile.transform[11] == 12.0);
    CHECK(tile.transform[12] == 13.0);
    CHECK(tile.transform[13] == 14.0);
    CHECK(tile.transform[14] == 15.0);
    CHECK(tile.transform[15] == 16.0);
  }

  SUBCASE("clobbers the existing transform") {
    glm::dmat4 transform(
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        glm::dvec4(13.0, 14.0, 15.0, 16.0));

    Cesium3DTiles::Tile tile;
    tile.transform = {
        101.0,
        102.0,
        103.0,
        104.0,
        105.0,
        106.0,
        107.0,
        108.0,
        109.0,
        110.0,
        111.0,
        112.0,
        113.0,
        114.0,
        115.0,
        116.0,
        117.0};
    Cesium3DTilesContent::TileTransform::setTransform(tile, transform);

    REQUIRE(tile.transform.size() == 16);
    CHECK(tile.transform[0] == 1.0);
    CHECK(tile.transform[1] == 2.0);
    CHECK(tile.transform[2] == 3.0);
    CHECK(tile.transform[3] == 4.0);
    CHECK(tile.transform[4] == 5.0);
    CHECK(tile.transform[5] == 6.0);
    CHECK(tile.transform[6] == 7.0);
    CHECK(tile.transform[7] == 8.0);
    CHECK(tile.transform[8] == 9.0);
    CHECK(tile.transform[9] == 10.0);
    CHECK(tile.transform[10] == 11.0);
    CHECK(tile.transform[11] == 12.0);
    CHECK(tile.transform[12] == 13.0);
    CHECK(tile.transform[13] == 14.0);
    CHECK(tile.transform[14] == 15.0);
    CHECK(tile.transform[15] == 16.0);
  }
}
