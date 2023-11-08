#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>

#include <catch2/catch.hpp>

#include <algorithm>

using namespace CesiumGeometry;
using namespace Cesium3DTilesContent;

TEST_CASE("ImplicitTilingUtilities child tile iteration") {
  SECTION("QuadtreeTileID") {
    QuadtreeTileID parent(1, 2, 3);

    QuadtreeChildren children = ImplicitTilingUtilities::getChildren(parent);

    // Check we can enumerate the children with a range-based for loop.
    int count = 0;
    for (const QuadtreeTileID& tileID : children) {
      CHECK(tileID.level == 2);
      CHECK((tileID.x == 4 || tileID.x == 5));
      CHECK((tileID.y == 6 || tileID.y == 7));
      ++count;
    }

    CHECK(count == 4);

    // Check we have exactly the right children.
    std::vector<QuadtreeTileID> expected{
        QuadtreeTileID(2, 4, 6),
        QuadtreeTileID(2, 5, 6),
        QuadtreeTileID(2, 4, 7),
        QuadtreeTileID(2, 5, 7)};
    auto mismatch = std::mismatch(
        children.begin(),
        children.end(),
        expected.begin(),
        expected.end());
    CHECK(mismatch.first == children.end());
    CHECK(mismatch.second == expected.end());
  }

  SECTION("OctreeTileID") {
    OctreeTileID parent(1, 2, 3, 4);

    OctreeChildren children = ImplicitTilingUtilities::getChildren(parent);

    // Check we can enumerate the children with a range-based for loop.
    int count = 0;
    for (const OctreeTileID& tileID : children) {
      CHECK(tileID.level == 2);
      CHECK((tileID.x == 4 || tileID.x == 5));
      CHECK((tileID.y == 6 || tileID.y == 7));
      CHECK((tileID.z == 8 || tileID.z == 9));
      ++count;
    }

    CHECK(count == 8);

    // Check we have exactly the right children.
    std::vector<OctreeTileID> expected{
        OctreeTileID(2, 4, 6, 8),
        OctreeTileID(2, 5, 6, 8),
        OctreeTileID(2, 4, 7, 8),
        OctreeTileID(2, 5, 7, 8),
        OctreeTileID(2, 4, 6, 9),
        OctreeTileID(2, 5, 6, 9),
        OctreeTileID(2, 4, 7, 9),
        OctreeTileID(2, 5, 7, 9)};
    auto mismatch = std::mismatch(
        children.begin(),
        children.end(),
        expected.begin(),
        expected.end());
    CHECK(mismatch.first == children.end());
    CHECK(mismatch.second == expected.end());
  }
}
