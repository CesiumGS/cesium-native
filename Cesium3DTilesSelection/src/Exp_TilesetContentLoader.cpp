#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
TilesetContentLoader::TilesetContentLoader(const TilesetExternals& externals)
    : _externals{externals} {}

void TilesetContentLoader::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions) {
  TileContent *pTileContent = tile.exp_GetContent();

  // create a user storage handle if it doesn't have any. This allows derived and
  // other processor to add custom user data to the tile if needed
  auto userStorageHandle = pTileContent->getLoaderCustomDataHandle();
  if (!_customDataStorage.isValidHandle(userStorageHandle)) {
    pTileContent->setLoaderCustomDataHandle(_customDataStorage.createHandle());
  }

  pTileContent->setState(TileLoadState::ContentLoading);
  doLoadTileContent(tile, contentOptions)
      .thenInMainThread([pTileContent](TileLoadResult&& result) {
        if (result.state == TileLoadState::ContentLoaded ||
            result.state == TileLoadState::Failed ||
            result.state == TileLoadState::FailedTemporarily) {
          TilesetContentLoader::setTileContentState(
              *pTileContent,
              std::move(result.contentKind),
              result.state,
              result.httpStatusCode);
        } else {
          // any states other than the three above are regarded as failed. We
          // will never allow derived class to influence the state of a tile
          TilesetContentLoader::setTileContentState(
              *pTileContent,
              std::move(result.contentKind),
              TileLoadState::Failed,
              result.httpStatusCode);
        }
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
    updateFailedState(tile);
    break;
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

void TilesetContentLoader::setTileFailedTemporarilyCallback(
    FailedTemporarilyTileCallback callback) {
  _failedTemporarilyCallback = std::move(callback);
}

void TilesetContentLoader::setTileContentState(
    TileContent& content,
    TileContentKind&& contentKind,
    TileLoadState state,
    uint16_t httpStatusCode) {
  content.setContentKind(std::move(contentKind));
  content.setState(state);
  content.setHttpStatusCode(httpStatusCode);
}

void TilesetContentLoader::resetTileContent(TileContent& content) {
  deleteAllTileUserData(content);
  content.setContentKind(TileUnknownContent{});
  content.setState(TileLoadState::Unloaded);
}

void TilesetContentLoader::deleteAllTileUserData(TileContent& content) {
  auto customDataHandle = content.getLoaderCustomDataHandle();
  if (_customDataStorage.isValidHandle(customDataHandle)) {
    _customDataStorage.destroyHandle(customDataHandle);
    content.setLoaderCustomDataHandle(TileUserDataStorage::NullHandle);
  }
}

void TilesetContentLoader::updateFailedState(
    Tile& tile)
{
  TileContent* pContent = tile.exp_GetContent();
  deleteAllTileUserData(*pContent);
}

void TilesetContentLoader::updateFailedTemporarilyState(
    Tile& tile)
{
  TileContent* pContent = tile.exp_GetContent();

  if (_failedTemporarilyCallback) {
    FailedTemporarilyTileAction action = _failedTemporarilyCallback(tile);
    if (action == FailedTemporarilyTileAction::GiveUp) {
      // move to failed state, so that the state machine will clean up itself automatically
      pContent->setState(TileLoadState::Failed);
    } else if (action == FailedTemporarilyTileAction::Retry) {
      // make sure that this tile has the correct invariant for an unloaded state
      resetTileContent(*pContent);
    }
  }
}

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
