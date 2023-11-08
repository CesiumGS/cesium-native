#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>

#include <catch2/catch.hpp>
#include <libmorton/morton.h>

#include <algorithm>

using namespace CesiumGeometry;
using namespace Cesium3DTilesContent;

TEST_CASE("ImplicitTilingUtilities child tile iteration") {
  SECTION("QuadtreeTileID") {
    QuadtreeTileID parent(11, 2, 3);

    QuadtreeChildren children = ImplicitTilingUtilities::getChildren(parent);

    // Check we can enumerate the children with a range-based for loop.
    int count = 0;
    for (const QuadtreeTileID& tileID : children) {
      CHECK(tileID.level == 12);
      CHECK((tileID.x == 4 || tileID.x == 5));
      CHECK((tileID.y == 6 || tileID.y == 7));
      ++count;
    }

    CHECK(count == 4);

    // Check we have exactly the right children.
    std::vector<QuadtreeTileID> expected{
        QuadtreeTileID(12, 4, 6),
        QuadtreeTileID(12, 5, 6),
        QuadtreeTileID(12, 4, 7),
        QuadtreeTileID(12, 5, 7)};
    auto mismatch = std::mismatch(
        children.begin(),
        children.end(),
        expected.begin(),
        expected.end());
    CHECK(mismatch.first == children.end());
    CHECK(mismatch.second == expected.end());
  }

  SECTION("OctreeTileID") {
    OctreeTileID parent(11, 2, 3, 4);

    OctreeChildren children = ImplicitTilingUtilities::getChildren(parent);

    // Check we can enumerate the children with a range-based for loop.
    int count = 0;
    for (const OctreeTileID& tileID : children) {
      CHECK(tileID.level == 12);
      CHECK((tileID.x == 4 || tileID.x == 5));
      CHECK((tileID.y == 6 || tileID.y == 7));
      CHECK((tileID.z == 8 || tileID.z == 9));
      ++count;
    }

    CHECK(count == 8);

    // Check we have exactly the right children.
    std::vector<OctreeTileID> expected{
        OctreeTileID(12, 4, 6, 8),
        OctreeTileID(12, 5, 6, 8),
        OctreeTileID(12, 4, 7, 8),
        OctreeTileID(12, 5, 7, 8),
        OctreeTileID(12, 4, 6, 9),
        OctreeTileID(12, 5, 6, 9),
        OctreeTileID(12, 4, 7, 9),
        OctreeTileID(12, 5, 7, 9)};
    auto mismatch = std::mismatch(
        children.begin(),
        children.end(),
        expected.begin(),
        expected.end());
    CHECK(mismatch.first == children.end());
    CHECK(mismatch.second == expected.end());
  }
}

TEST_CASE("ImplicitTilingUtilities::resolveUrl") {
  SECTION("quadtree") {
    QuadtreeTileID tileID(11, 2, 3);
    std::string url = ImplicitTilingUtilities::resolveUrl(
        "https://example.com",
        "tiles/{level}/{x}/{y}",
        tileID);
    CHECK(url == "https://example.com/tiles/11/2/3");
  }

  SECTION("octree") {
    OctreeTileID tileID(11, 2, 3, 4);
    std::string url = ImplicitTilingUtilities::resolveUrl(
        "https://example.com",
        "tiles/{level}/{x}/{y}/{z}",
        tileID);
    CHECK(url == "https://example.com/tiles/11/2/3/4");
  }
}

TEST_CASE("ImplicitTilingUtilities::computeMortonIndex") {
  SECTION("quadtree") {
    QuadtreeTileID tileID(11, 2, 3);
    CHECK(
        ImplicitTilingUtilities::computeMortonIndex(tileID) ==
        libmorton::morton2D_64_encode(2, 3));
  }

  SECTION("quadtree") {
    OctreeTileID tileID(11, 2, 3, 4);
    CHECK(
        ImplicitTilingUtilities::computeMortonIndex(tileID) ==
        libmorton::morton3D_64_encode(2, 3, 4));
  }
}

TEST_CASE("ImplicitTilingUtilities::computeRelativeMortonIndex") {
  SECTION("quadtree") {
    QuadtreeTileID rootID(11, 2, 3);
    QuadtreeTileID tileID(12, 5, 6);
    CHECK(
        ImplicitTilingUtilities::computeRelativeMortonIndex(rootID, tileID) ==
        1);
  }

  SECTION("octree") {
    OctreeTileID rootID(11, 2, 3, 4);
    OctreeTileID tileID(12, 5, 6, 8);
    CHECK(
        ImplicitTilingUtilities::computeRelativeMortonIndex(rootID, tileID) ==
        1);
  }
}

TEST_CASE("ImplicitTilingUtilities::getSubtreeRootID") {
  SECTION("quadtree") {
    QuadtreeTileID tileID(10, 2, 3);
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(5, tileID) ==
        QuadtreeTileID(10, 2, 3));
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(4, tileID) ==
        QuadtreeTileID(8, 0, 0));
  }

  SECTION("octree") {
    OctreeTileID tileID(10, 2, 3, 4);
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(5, tileID) ==
        OctreeTileID(10, 2, 3, 4));
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(4, tileID) ==
        OctreeTileID(8, 0, 0, 1));
  }
}

TEST_CASE("ImplicitTilingUtilities::absoluteTileIDToRelative") {
  SECTION("quadtree") {
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            QuadtreeTileID(0, 0, 0),
            QuadtreeTileID(11, 2, 3)) == QuadtreeTileID(11, 2, 3));
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            QuadtreeTileID(11, 2, 3),
            QuadtreeTileID(11, 2, 3)) == QuadtreeTileID(0, 0, 0));
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            QuadtreeTileID(11, 2, 3),
            QuadtreeTileID(12, 5, 7)) == QuadtreeTileID(1, 1, 1));
  }

  SECTION("octree") {
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            OctreeTileID(0, 0, 0, 0),
            OctreeTileID(11, 2, 3, 4)) == OctreeTileID(11, 2, 3, 4));
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            OctreeTileID(11, 2, 3, 4),
            OctreeTileID(11, 2, 3, 4)) == OctreeTileID(0, 0, 0, 0));
    CHECK(
        ImplicitTilingUtilities::absoluteTileIDToRelative(
            OctreeTileID(11, 2, 3, 4),
            OctreeTileID(12, 5, 7, 9)) == OctreeTileID(1, 1, 1, 1));
  }
}

TEST_CASE("ImplicitTilingUtilities::computeLevelDenominator") {
  CHECK(ImplicitTilingUtilities::computeLevelDenominator(0) == 1.0);
  CHECK(ImplicitTilingUtilities::computeLevelDenominator(1) == 2.0);
  CHECK(ImplicitTilingUtilities::computeLevelDenominator(2) == 4.0);
}
