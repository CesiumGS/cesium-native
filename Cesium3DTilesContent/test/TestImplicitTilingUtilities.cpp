#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/TileBoundingVolumes.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double3x3.hpp>
#include <libmorton/morton.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesContent;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("ImplicitTilingUtilities child tile iteration") {
  SUBCASE("QuadtreeTileID") {
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

  SUBCASE("OctreeTileID") {
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
  SUBCASE("quadtree") {
    QuadtreeTileID tileID(11, 2, 3);
    std::string url = ImplicitTilingUtilities::resolveUrl(
        "https://example.com",
        "tiles/{level}/{x}/{y}",
        tileID);
    CHECK(url == "https://example.com/tiles/11/2/3");
  }

  SUBCASE("octree") {
    OctreeTileID tileID(11, 2, 3, 4);
    std::string url = ImplicitTilingUtilities::resolveUrl(
        "https://example.com",
        "tiles/{level}/{x}/{y}/{z}",
        tileID);
    CHECK(url == "https://example.com/tiles/11/2/3/4");
  }
}

TEST_CASE("ImplicitTilingUtilities::computeMortonIndex") {
  SUBCASE("quadtree") {
    QuadtreeTileID tileID(11, 2, 3);
    CHECK(
        ImplicitTilingUtilities::computeMortonIndex(tileID) ==
        libmorton::morton2D_64_encode(2, 3));
  }

  SUBCASE("quadtree") {
    OctreeTileID tileID(11, 2, 3, 4);
    CHECK(
        ImplicitTilingUtilities::computeMortonIndex(tileID) ==
        libmorton::morton3D_64_encode(2, 3, 4));
  }
}

TEST_CASE("ImplicitTilingUtilities::computeRelativeMortonIndex") {
  SUBCASE("quadtree") {
    QuadtreeTileID rootID(11, 2, 3);
    QuadtreeTileID tileID(12, 5, 6);
    CHECK(
        ImplicitTilingUtilities::computeRelativeMortonIndex(rootID, tileID) ==
        1);
  }

  SUBCASE("octree") {
    OctreeTileID rootID(11, 2, 3, 4);
    OctreeTileID tileID(12, 5, 6, 8);
    CHECK(
        ImplicitTilingUtilities::computeRelativeMortonIndex(rootID, tileID) ==
        1);
  }
}

TEST_CASE("ImplicitTilingUtilities::getSubtreeRootID") {
  SUBCASE("quadtree") {
    QuadtreeTileID tileID(10, 2, 3);
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(5, tileID) ==
        QuadtreeTileID(10, 2, 3));
    CHECK(
        ImplicitTilingUtilities::getSubtreeRootID(4, tileID) ==
        QuadtreeTileID(8, 0, 0));
  }

  SUBCASE("octree") {
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
  SUBCASE("quadtree") {
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

  SUBCASE("octree") {
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

TEST_CASE("ImplicitTilingUtilities::computeBoundingVolume") {
  SUBCASE("OrientedBoundingBox") {
    SUBCASE("quadtree") {
      OrientedBoundingBox root(glm::dvec3(1.0, 2.0, 3.0), glm::dmat3(10.0));

      OrientedBoundingBox l1x0y0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 0, 0));
      CHECK(l1x0y0.getCenter() == glm::dvec3(-4.0, -3.0, 3.0));
      CHECK(l1x0y0.getLengths() == glm::dvec3(10.0, 10.0, 20.0));

      OrientedBoundingBox l1x1y0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 1, 0));
      CHECK(l1x1y0.getCenter() == glm::dvec3(6.0, -3.0, 3.0));
      CHECK(l1x1y0.getLengths() == glm::dvec3(10.0, 10.0, 20.0));

      OrientedBoundingBox l1x0y1 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 0, 1));
      CHECK(l1x0y1.getCenter() == glm::dvec3(-4.0, 7.0, 3.0));
      CHECK(l1x0y1.getLengths() == glm::dvec3(10.0, 10.0, 20.0));
    }

    SUBCASE("octree") {
      OrientedBoundingBox root(glm::dvec3(1.0, 2.0, 3.0), glm::dmat3(10.0));

      OrientedBoundingBox l1x0y0z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 0, 0));
      CHECK(l1x0y0z0.getCenter() == glm::dvec3(-4.0, -3.0, -2.0));
      CHECK(l1x0y0z0.getLengths() == glm::dvec3(10.0, 10.0, 10.0));

      OrientedBoundingBox l1x1y0z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 1, 0, 0));
      CHECK(l1x1y0z0.getCenter() == glm::dvec3(6.0, -3.0, -2.0));
      CHECK(l1x1y0z0.getLengths() == glm::dvec3(10.0, 10.0, 10.0));

      OrientedBoundingBox l1x0y1z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 1, 0));
      CHECK(l1x0y1z0.getCenter() == glm::dvec3(-4.0, 7.0, -2.0));
      CHECK(l1x0y1z0.getLengths() == glm::dvec3(10.0, 10.0, 10.0));

      OrientedBoundingBox l1x0y0z1 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 0, 1));
      CHECK(l1x0y0z1.getCenter() == glm::dvec3(-4.0, -3.0, 8.0));
      CHECK(l1x0y0z1.getLengths() == glm::dvec3(10.0, 10.0, 10.0));
    }
  }

  SUBCASE("BoundingRegion") {
    SUBCASE("quadtree") {
      BoundingRegion root(
          GlobeRectangle(1.0, 2.0, 3.0, 4.0),
          10.0,
          20.0,
          Ellipsoid::WGS84);

      BoundingRegion l1x0y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 0, 0),
          Ellipsoid::WGS84);
      CHECK(l1x0y0.getRectangle().getWest() == 1.0);
      CHECK(l1x0y0.getRectangle().getSouth() == 2.0);
      CHECK(l1x0y0.getRectangle().getEast() == 2.0);
      CHECK(l1x0y0.getRectangle().getNorth() == 3.0);
      CHECK(l1x0y0.getMinimumHeight() == 10.0);
      CHECK(l1x0y0.getMaximumHeight() == 20.0);

      BoundingRegion l1x1y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 1, 0),
          Ellipsoid::WGS84);
      CHECK(l1x1y0.getRectangle().getWest() == 2.0);
      CHECK(l1x1y0.getRectangle().getSouth() == 2.0);
      CHECK(l1x1y0.getRectangle().getEast() == 3.0);
      CHECK(l1x1y0.getRectangle().getNorth() == 3.0);
      CHECK(l1x1y0.getMinimumHeight() == 10.0);
      CHECK(l1x1y0.getMaximumHeight() == 20.0);

      BoundingRegion l1x0y1 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 0, 1),
          Ellipsoid::WGS84);
      CHECK(l1x0y1.getRectangle().getWest() == 1.0);
      CHECK(l1x0y1.getRectangle().getSouth() == 3.0);
      CHECK(l1x0y1.getRectangle().getEast() == 2.0);
      CHECK(l1x0y1.getRectangle().getNorth() == 4.0);
      CHECK(l1x0y1.getMinimumHeight() == 10.0);
      CHECK(l1x0y1.getMaximumHeight() == 20.0);
    }

    SUBCASE("octree") {
      BoundingRegion root(
          GlobeRectangle(1.0, 2.0, 3.0, 4.0),
          10.0,
          20.0,
          Ellipsoid::WGS84);

      BoundingRegion l1x0y0z0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 0, 0, 0),
          Ellipsoid::WGS84);
      CHECK(l1x0y0z0.getRectangle().getWest() == 1.0);
      CHECK(l1x0y0z0.getRectangle().getSouth() == 2.0);
      CHECK(l1x0y0z0.getRectangle().getEast() == 2.0);
      CHECK(l1x0y0z0.getRectangle().getNorth() == 3.0);
      CHECK(l1x0y0z0.getMinimumHeight() == 10.0);
      CHECK(l1x0y0z0.getMaximumHeight() == 15.0);

      BoundingRegion l1x1y0z0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 1, 0, 0),
          Ellipsoid::WGS84);
      CHECK(l1x1y0z0.getRectangle().getWest() == 2.0);
      CHECK(l1x1y0z0.getRectangle().getSouth() == 2.0);
      CHECK(l1x1y0z0.getRectangle().getEast() == 3.0);
      CHECK(l1x1y0z0.getRectangle().getNorth() == 3.0);
      CHECK(l1x1y0z0.getMinimumHeight() == 10.0);
      CHECK(l1x1y0z0.getMaximumHeight() == 15.0);

      BoundingRegion l1x0y1z0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 0, 1, 0),
          Ellipsoid::WGS84);
      CHECK(l1x0y1z0.getRectangle().getWest() == 1.0);
      CHECK(l1x0y1z0.getRectangle().getSouth() == 3.0);
      CHECK(l1x0y1z0.getRectangle().getEast() == 2.0);
      CHECK(l1x0y1z0.getRectangle().getNorth() == 4.0);
      CHECK(l1x0y1z0.getMinimumHeight() == 10.0);
      CHECK(l1x0y1z0.getMaximumHeight() == 15.0);

      BoundingRegion l1x0y0z1 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 0, 0, 1),
          Ellipsoid::WGS84);
      CHECK(l1x0y0z1.getRectangle().getWest() == 1.0);
      CHECK(l1x0y0z1.getRectangle().getSouth() == 2.0);
      CHECK(l1x0y0z1.getRectangle().getEast() == 2.0);
      CHECK(l1x0y0z1.getRectangle().getNorth() == 3.0);
      CHECK(l1x0y0z1.getMinimumHeight() == 15.0);
      CHECK(l1x0y0z1.getMaximumHeight() == 20.0);
    }
  }

  SUBCASE("S2") {
    SUBCASE("quadtree") {
      S2CellBoundingVolume root(
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(0, 0, 0)),
          10.0,
          20.0,
          Ellipsoid::WGS84);

      S2CellBoundingVolume l1x0y0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 0, 0),
              Ellipsoid::WGS84);
      CHECK(l1x0y0.getCellID().getFace() == 1);
      CHECK(
          l1x0y0.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 0)).getID());
      CHECK(l1x0y0.getMinimumHeight() == 10.0);
      CHECK(l1x0y0.getMaximumHeight() == 20.0);

      S2CellBoundingVolume l1x1y0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 1, 0),
              Ellipsoid::WGS84);
      CHECK(l1x1y0.getCellID().getFace() == 1);
      CHECK(
          l1x1y0.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 1, 0)).getID());
      CHECK(l1x1y0.getMinimumHeight() == 10.0);
      CHECK(l1x1y0.getMaximumHeight() == 20.0);

      S2CellBoundingVolume l1x0y1 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              QuadtreeTileID(1, 0, 1),
              Ellipsoid::WGS84);
      CHECK(l1x0y1.getCellID().getFace() == 1);
      CHECK(
          l1x0y1.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 1)).getID());
      CHECK(l1x0y1.getMinimumHeight() == 10.0);
      CHECK(l1x0y1.getMaximumHeight() == 20.0);
    }

    SUBCASE("octree") {
      S2CellBoundingVolume root(
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(0, 0, 0)),
          10.0,
          20.0,
          Ellipsoid::WGS84);

      S2CellBoundingVolume l1x0y0z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 0, 0),
              Ellipsoid::WGS84);
      CHECK(l1x0y0z0.getCellID().getFace() == 1);
      CHECK(
          l1x0y0z0.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 0)).getID());
      CHECK(l1x0y0z0.getMinimumHeight() == 10.0);
      CHECK(l1x0y0z0.getMaximumHeight() == 15.0);

      S2CellBoundingVolume l1x1y0z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 1, 0, 0),
              Ellipsoid::WGS84);
      CHECK(l1x1y0z0.getCellID().getFace() == 1);
      CHECK(
          l1x1y0z0.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 1, 0)).getID());
      CHECK(l1x1y0z0.getMinimumHeight() == 10.0);
      CHECK(l1x1y0z0.getMaximumHeight() == 15.0);

      S2CellBoundingVolume l1x0y1z0 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 1, 0),
              Ellipsoid::WGS84);
      CHECK(l1x0y1z0.getCellID().getFace() == 1);
      CHECK(
          l1x0y1z0.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 1)).getID());
      CHECK(l1x0y1z0.getMinimumHeight() == 10.0);
      CHECK(l1x0y1z0.getMaximumHeight() == 15.0);

      S2CellBoundingVolume l1x0y0z1 =
          ImplicitTilingUtilities::computeBoundingVolume(
              root,
              OctreeTileID(1, 0, 0, 1),
              Ellipsoid::WGS84);
      CHECK(l1x0y0z1.getCellID().getFace() == 1);
      CHECK(
          l1x0y0z1.getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 0)).getID());
      CHECK(l1x0y0z1.getMinimumHeight() == 15.0);
      CHECK(l1x0y0z1.getMaximumHeight() == 20.0);
    }
  }

  SUBCASE("BoundingVolume") {
    SUBCASE("quadtree") {
      BoundingVolume root{};

      TileBoundingVolumes::setOrientedBoundingBox(
          root,
          OrientedBoundingBox(glm::dvec3(1.0, 2.0, 3.0), glm::dmat3(10.0)));
      TileBoundingVolumes::setBoundingRegion(
          root,
          BoundingRegion(
              GlobeRectangle(1.0, 2.0, 3.0, 4.0),
              10.0,
              20.0,
              Ellipsoid::WGS84));
      TileBoundingVolumes::setS2CellBoundingVolume(
          root,
          S2CellBoundingVolume(
              S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(0, 0, 0)),
              10.0,
              20.0,
              Ellipsoid::WGS84));

      BoundingVolume l1x0y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 0, 0),
          Ellipsoid::WGS84);
      std::optional<OrientedBoundingBox> maybeBox =
          TileBoundingVolumes::getOrientedBoundingBox(l1x0y0);
      REQUIRE(maybeBox);
      CHECK(maybeBox->getCenter() == glm::dvec3(-4.0, -3.0, 3.0));
      CHECK(maybeBox->getLengths() == glm::dvec3(10.0, 10.0, 20.0));

      BoundingVolume l1x1y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 1, 0),
          Ellipsoid::WGS84);
      std::optional<BoundingRegion> maybeRegion =
          TileBoundingVolumes::getBoundingRegion(l1x1y0, Ellipsoid::WGS84);
      REQUIRE(maybeRegion);
      CHECK(maybeRegion->getRectangle().getWest() == 2.0);
      CHECK(maybeRegion->getRectangle().getSouth() == 2.0);
      CHECK(maybeRegion->getRectangle().getEast() == 3.0);
      CHECK(maybeRegion->getRectangle().getNorth() == 3.0);
      CHECK(maybeRegion->getMinimumHeight() == 10.0);
      CHECK(maybeRegion->getMaximumHeight() == 20.0);

      BoundingVolume l1x0y1 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          QuadtreeTileID(1, 0, 1),
          Ellipsoid::WGS84);
      std::optional<S2CellBoundingVolume> maybeS2 =
          TileBoundingVolumes::getS2CellBoundingVolume(
              l1x0y1,
              Ellipsoid::WGS84);
      REQUIRE(maybeS2);
      CHECK(maybeS2->getCellID().getFace() == 1);
      CHECK(
          maybeS2->getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 1)).getID());
      CHECK(maybeS2->getMinimumHeight() == 10.0);
      CHECK(maybeS2->getMaximumHeight() == 20.0);
    }

    SUBCASE("octree") {
      BoundingVolume root{};

      TileBoundingVolumes::setOrientedBoundingBox(
          root,
          OrientedBoundingBox(glm::dvec3(1.0, 2.0, 3.0), glm::dmat3(10.0)));
      TileBoundingVolumes::setBoundingRegion(
          root,
          BoundingRegion(
              GlobeRectangle(1.0, 2.0, 3.0, 4.0),
              10.0,
              20.0,
              Ellipsoid::WGS84));
      TileBoundingVolumes::setS2CellBoundingVolume(
          root,
          S2CellBoundingVolume(
              S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(0, 0, 0)),
              10.0,
              20.0,
              Ellipsoid::WGS84));

      BoundingVolume l1x0y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 0, 0, 0),
          Ellipsoid::WGS84);
      std::optional<OrientedBoundingBox> maybeBox =
          TileBoundingVolumes::getOrientedBoundingBox(l1x0y0);
      REQUIRE(maybeBox);
      CHECK(maybeBox->getCenter() == glm::dvec3(-4.0, -3.0, -2.0));
      CHECK(maybeBox->getLengths() == glm::dvec3(10.0, 10.0, 10.0));

      BoundingVolume l1x1y0 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 1, 0, 0),
          Ellipsoid::WGS84);
      std::optional<BoundingRegion> maybeRegion =
          TileBoundingVolumes::getBoundingRegion(l1x1y0, Ellipsoid::WGS84);
      REQUIRE(maybeRegion);
      CHECK(maybeRegion->getRectangle().getWest() == 2.0);
      CHECK(maybeRegion->getRectangle().getSouth() == 2.0);
      CHECK(maybeRegion->getRectangle().getEast() == 3.0);
      CHECK(maybeRegion->getRectangle().getNorth() == 3.0);
      CHECK(maybeRegion->getMinimumHeight() == 10.0);
      CHECK(maybeRegion->getMaximumHeight() == 15.0);

      BoundingVolume l1x0y1 = ImplicitTilingUtilities::computeBoundingVolume(
          root,
          OctreeTileID(1, 0, 1, 0),
          Ellipsoid::WGS84);
      std::optional<S2CellBoundingVolume> maybeS2 =
          TileBoundingVolumes::getS2CellBoundingVolume(
              l1x0y1,
              Ellipsoid::WGS84);
      REQUIRE(maybeS2);
      CHECK(maybeS2->getCellID().getFace() == 1);
      CHECK(
          maybeS2->getCellID().getID() ==
          S2CellID::fromQuadtreeTileID(1, QuadtreeTileID(1, 0, 1)).getID());
      CHECK(maybeS2->getMinimumHeight() == 10.0);
      CHECK(maybeS2->getMaximumHeight() == 15.0);
    }
  }
}
