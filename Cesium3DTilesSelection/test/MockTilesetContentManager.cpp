#include "MockTilesetContentManager.h"

namespace Cesium3DTilesSelection {
void MockTilesetContentManagerTestFixture::setTileLoadState(
    Cesium3DTilesSelection::Tile& tile,
    Cesium3DTilesSelection::TileLoadState loadState) {
  tile.setState(loadState);
}

void MockTilesetContentManagerTestFixture::setTileShouldContinueUpdating(
    Cesium3DTilesSelection::Tile& tile,
    bool shouldContinueUpdating) {
  tile.setContentShouldContinueUpdating(shouldContinueUpdating);
}
} // namespace Cesium3DTilesSelection
