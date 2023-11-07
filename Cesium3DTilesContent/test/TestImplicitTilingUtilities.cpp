#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>

#include <catch2/catch.hpp>

using namespace CesiumGeometry;
using namespace Cesium3DTilesContent;

TEST_CASE("ImplicitTilingUtilities child tile iteration") {
  SECTION("QuadtreeTileID") {
    QuadtreeTileID parent(1, 2, 3);

    QuadtreeChildIterator it = ImplicitTilingUtilities::childrenBegin(parent);

    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 6);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 6);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 7);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 7);

    ++it;
    CHECK(it == ImplicitTilingUtilities::childrenEnd(parent));
  }

  SECTION("OctreeTileID") {
    OctreeTileID parent(1, 2, 3, 4);

    OctreeChildIterator it = ImplicitTilingUtilities::childrenBegin(parent);

    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 6);
    CHECK(it->z == 8);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 6);
    CHECK(it->z == 8);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 7);
    CHECK(it->z == 8);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 7);
    CHECK(it->z == 8);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 6);
    CHECK(it->z == 9);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 6);
    CHECK(it->z == 9);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 4);
    CHECK(it->y == 7);
    CHECK(it->z == 9);

    ++it;
    REQUIRE(it != ImplicitTilingUtilities::childrenEnd(parent));
    CHECK(it->level == 2);
    CHECK(it->x == 5);
    CHECK(it->y == 7);
    CHECK(it->z == 9);

    ++it;
    CHECK(it == ImplicitTilingUtilities::childrenEnd(parent));
  }
}
