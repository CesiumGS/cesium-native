#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
struct TileLoadResultAndRenderResources {
  TileLoadResult result;
  void* pRenderResources{nullptr};
};
} // namespace

TilesetContentLoader::TilesetContentLoader(const TilesetExternals& externals)
    : _externals{externals} {}

void TilesetContentLoader::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions) {
  TileContent* pContent = tile.exp_GetContent();

  // create a user storage handle if it doesn't have any. This allows derived
  // and other processor to add custom user data to the tile if needed
  auto userStorageHandle = pContent->getLoaderCustomDataHandle();
  if (!_customDataStorage.isValidHandle(userStorageHandle)) {
    pContent->setLoaderCustomDataHandle(_customDataStorage.createHandle());
  }

  pContent->setState(TileLoadState::ContentLoading);
  doLoadTileContent(tile, contentOptions)
      .thenInWorkerThread(
          [pPrepareRendererResources = _externals.pPrepareRendererResources,
           transform = tile.getTransform()](TileLoadResult&& result) {
            void* pRenderResources = nullptr;
            if (result.state == TileLoadState::ContentLoaded) {
              TileRenderContent* pRenderContent =
                  std::get_if<TileRenderContent>(&result.contentKind);
              if (pRenderContent && pRenderContent->model) {
                pRenderResources =
                    pPrepareRendererResources->prepareInLoadThread(
                        *pRenderContent->model,
                        transform);
              }
            }

            return TileLoadResultAndRenderResources{
                std::move(result),
                pRenderResources};
          })
      .thenInMainThread(
          [pContent](TileLoadResultAndRenderResources&& pair) {
            TileLoadResult& result = pair.result;
            if (result.state == TileLoadState::ContentLoaded ||
                result.state == TileLoadState::Failed ||
                result.state == TileLoadState::FailedTemporarily) {
              TilesetContentLoader::setTileContentState(
                  *pContent,
                  std::move(result.contentKind),
                  result.state,
                  result.httpStatusCode,
                  pair.pRenderResources);
            } else {
              // any states other than the three above are regarded as failed.
              // We will never allow derived class to influence the state of a
              // tile
              TilesetContentLoader::setTileContentState(
                  *pContent,
                  std::move(result.contentKind),
                  TileLoadState::Failed,
                  result.httpStatusCode,
                  nullptr);
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
  if (state == TileLoadState::Unloaded) {
    return true;
  }

  if (state == TileLoadState::ContentLoading) {
    return false;
  }

  if (!doUnloadTileContent(tile)) {
    return false;
  }

  switch (state) {
  case TileLoadState::ContentLoaded:
    unloadContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    unloadDoneState(tile);
    break;
  default:
    resetTileContent(*pContent);
  }

  return true;
}

void TilesetContentLoader::setTileFailedTemporarilyCallback(
    FailedTemporarilyTileCallback callback) {
  _failedTemporarilyCallback = std::move(callback);
}

void TilesetContentLoader::setTileContentState(
    TileContent& content,
    TileContentKind&& contentKind,
    TileLoadState state,
    uint16_t httpStatusCode,
    void* pRenderResources) {
  content.setContentKind(std::move(contentKind));
  content.setState(state);
  content.setHttpStatusCode(httpStatusCode);
  content.setRenderResources(pRenderResources);
}

void TilesetContentLoader::resetTileContent(TileContent& content) {
  deleteAllTileUserData(content);
  content.setContentKind(TileUnknownContent{});
  content.setState(TileLoadState::Unloaded);
  content.setHttpStatusCode(0);
}

void TilesetContentLoader::deleteAllTileUserData(TileContent& content) {
  auto customDataHandle = content.getLoaderCustomDataHandle();
  if (_customDataStorage.isValidHandle(customDataHandle)) {
    _customDataStorage.destroyHandle(customDataHandle);
    content.setLoaderCustomDataHandle(TileUserDataStorage::NullHandle);
  }
}

void TilesetContentLoader::updateFailedTemporarilyState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();

  if (_failedTemporarilyCallback) {
    FailedTemporarilyTileAction action = _failedTemporarilyCallback(tile);
    if (action == FailedTemporarilyTileAction::GiveUp) {
      // move to failed state
      deleteAllTileUserData(*pContent);
      pContent->setState(TileLoadState::Failed);
    } else if (action == FailedTemporarilyTileAction::Retry) {
      // make sure that this tile has the correct invariant for an unloaded
      // state
      resetTileContent(*pContent);
    }
  }
}

void TilesetContentLoader::updateContentLoadedState(Tile& tile) {
  doProcessLoadedContent(tile);

  // create render resources in the main thread
  TileContent* pContent = tile.exp_GetContent();
  TileRenderContent* pRenderContent = pContent->getRenderContent();
  if (pRenderContent && pRenderContent->model) {
    void* pWorkerRenderResources = pContent->getRenderResources();
    void* pMainThreadRenderResources =
        _externals.pPrepareRendererResources->prepareInMainThread(
            tile,
            pWorkerRenderResources);
    pContent->setRenderResources(pMainThreadRenderResources);
  }

  pContent->setState(TileLoadState::Done);
}

void TilesetContentLoader::updateDoneState(Tile& tile) {
  doUpdateTileContent(tile);
}

void TilesetContentLoader::unloadContentLoadedState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  void* pWorkerRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);

  resetTileContent(*pContent);
}

void TilesetContentLoader::unloadDoneState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  void* pMainThreadRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);

  resetTileContent(*pContent);
}
} // namespace Cesium3DTilesSelection
