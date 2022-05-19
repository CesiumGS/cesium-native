#include <Cesium3DTilesSelection/Exp_ImplicitTilesetGenerator.h>
#include <catch2/catch.hpp>

TEST_CASE("Test generator") {
  Cesium3DTilesSelection::Octree octree(5, 5);
  octree.markTileEmptyContent(CesiumGeometry::OctreeTileID{0, 0, 0, 0});
  octree.markTileEmptyContent(CesiumGeometry::OctreeTileID{1, 0, 0, 0});
  octree.markTileEmptyContent(CesiumGeometry::OctreeTileID{1, 1, 0, 1});
  octree.markTileEmptyContent(CesiumGeometry::OctreeTileID{1, 1, 1, 1});

  std::filesystem::create_directories(std::filesystem::path(Cesium3DTilesSelection_TEST_DATA_DIR) / "octree");

  Cesium3DTilesSelection::ImplicitSerializer serializer;
  serializer.serializeOctree(
      octree,
      std::filesystem::path(Cesium3DTilesSelection_TEST_DATA_DIR) / "octree");
}
