#include "MockTilesetContentManager.h"

namespace Cesium3DTilesSelection {
void MockTilesetContentManagerTestFixture::setTileLoadState(
    Cesium3DTilesSelection::Tile& tile,
    Cesium3DTilesSelection::TileLoadState loadState) {
  tile.setState(loadState);
}

void MockTilesetContentManagerTestFixture::setTileShouldContinueUpdated(
    Cesium3DTilesSelection::Tile& tile,
    bool shouldContinueUpdated) {
  tile.setContentShouldContinueUpdated(shouldContinueUpdated);
}
} // namespace Cesium3DTilesSelection
