#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
void TilesetContentLoader::loadTileContent(
    Tile& tile,
    CesiumAsync::AsyncSystem& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    std::shared_ptr<spdlog::logger>& pLogger,
    const TilesetContentOptions& contentOptions) {
  TileContentLoadInfo
      loadInfo{asyncSystem, pAssetAccessor, pLogger, contentOptions, tile};

  std::shared_ptr<TileContent> pTileContent =
      std::make_shared<TileContent>(this);
  tile.exp_SetContent(pTileContent);
  doLoadTileContent(loadInfo).thenInMainThread(
      [pTileContent](TileContentKind&& contentKind) {
        TilesetContentLoader::setTileContentState(
            *pTileContent,
            std::move(contentKind),
            TileLoadState::ContentLoaded);
      });
}

void TilesetContentLoader::updateTileContent(Tile& tile) {
  const TileContent* pContent = tile.exp_GetContent();
  if (!pContent) {
    return;
  }

  TileLoadState state = pContent->getState();
  switch (state) {
  case TileLoadState::Failed:
    break;
  case TileLoadState::FailedTemporarily:
    break;
  case TileLoadState::Unloaded:
    break;
  case TileLoadState::ContentLoading:
    break;
  case TileLoadState::ContentLoaded:
    processContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    processDoneState(tile);
    break;
  default:
    break;
  }
}

void TilesetContentLoader::setTileContentState(TileContent& content, TileContentKind&& contentKind, TileLoadState state) {
  content.setContentKind(std::move(contentKind));
  content.setState(state);
}

void TilesetContentLoader::processContentLoadedState(Tile& tile) {
  doProcessLoadedContent(tile);

  TileContent *pTileContent = tile.exp_GetContent(); 
  pTileContent->setState(TileLoadState::Done);
}

void TilesetContentLoader::processDoneState(Tile& tile) {
  doUpdateTileContent(tile);
}
} // namespace Cesium3DTilesSelection
