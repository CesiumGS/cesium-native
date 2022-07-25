#pragma once

#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {
class MockTilesetContentManagerTestFixture {
public:
  static void setTileLoadState(
      Cesium3DTilesSelection::Tile& tile,
      Cesium3DTilesSelection::TileLoadState loadState);

  static void setTileShouldContinueUpdated(
      Cesium3DTilesSelection::Tile& tile,
      bool shouldContinueToBeUpdated);
};
} // namespace Cesium3DTilesSelection
