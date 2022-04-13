#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
TilesetContentLoader::TilesetContentLoader(const TilesetExternals& externals)
    : _externals{externals} {}

void TilesetContentLoader::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions) {
  TileContentLoadInfo loadInfo{
      _externals.asyncSystem,
      _externals.pAssetAccessor,
      _externals.pLogger,
      contentOptions,
      tile};

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
  case TileLoadState::FailedTemporarily:
    updateFailedTemporarilyState(tile);
    break;
  case TileLoadState::ContentLoaded:
    updateContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    updateDoneState(tile);
    break;
  default:
    break;
  }
}

bool TilesetContentLoader::unloadTileContent(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  if (!pContent) {
    return true;
  }

  TileLoadState state = pContent->getState();
  switch (state) {
  case TileLoadState::ContentLoading:
    return false;
  case TileLoadState::ContentLoaded:
    return unloadContentLoadedState(tile);
  case TileLoadState::Done:
    return unloadDoneState(tile);
  default:
    resetTileContent(*pContent);
    return true;
  }
}

void TilesetContentLoader::setTileContentState(
    TileContent& content,
    TileContentKind&& contentKind,
    TileLoadState state) {
  content.setContentKind(std::move(contentKind));
  content.setState(state);
}

void TilesetContentLoader::resetTileContent(TileContent& content) {
  content.setContentKind(TileUnknownContent{});
  content.setState(TileLoadState::Unloaded);
}

void TilesetContentLoader::updateFailedTemporarilyState(
    [[maybe_unused]] Tile& tile) {}

void TilesetContentLoader::updateContentLoadedState(Tile& tile) {
  doProcessLoadedContent(tile);

  TileContent* pTileContent = tile.exp_GetContent();
  pTileContent->setState(TileLoadState::Done);
}

void TilesetContentLoader::updateDoneState(Tile& tile) {
  doUpdateTileContent(tile);
}

bool TilesetContentLoader::unloadContentLoadedState(
    [[maybe_unused]] Tile& tile) {
  return true;
}

bool TilesetContentLoader::unloadDoneState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  if (doUnloadTileContent(tile)) {
    resetTileContent(*pContent);
    return true;
  }

  return false;
}
} // namespace Cesium3DTilesSelection
