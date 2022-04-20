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

  if (pContent->getState() != TileLoadState::Unloaded &&
      pContent->getState() != TileLoadState::FailedTemporarily) {
    return;
  }

  // create a user storage handle if it doesn't have any. This allows derived
  // and other processors to add custom user data to the tile if needed
  auto userStorageHandle = pContent->getLoaderCustomDataHandle();
  if (!_customDataStorage.isValidHandle(userStorageHandle)) {
    pContent->setLoaderCustomDataHandle(_customDataStorage.createHandle());
  }

  pContent->setState(TileLoadState::ContentLoading);
  doLoadTileContent(tile, contentOptions, {})
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
      .thenInMainThread([pContent](TileLoadResultAndRenderResources&& pair) {
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
  case TileLoadState::ContentLoaded:
    updateContentLoadedState(tile);
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

  switch (state) {
  case TileLoadState::ContentLoaded:
    unloadContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    unloadDoneState(tile);
    break;
  default:
    break;
  }

  // we will only ever remove all user data when this tile is unloaded
  auto customDataHandle = pContent->getLoaderCustomDataHandle();
  if (_customDataStorage.isValidHandle(customDataHandle)) {
    _customDataStorage.destroyHandle(customDataHandle);
    pContent->setLoaderCustomDataHandle(TileUserDataStorage::NullHandle);
  }

  pContent->setContentKind(TileUnknownContent{});
  pContent->setHttpStatusCode(0);
  pContent->setState(TileLoadState::Unloaded);
  return true;
}

void TilesetContentLoader::setRequestHeadersChangeListenser(
  std::function<void(CesiumAsync::IAssetAccessor::THeader&& changedHeader)>
  listener) {
  _requestHeadersChangeListener = std::move(listener);
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

void TilesetContentLoader::unloadContentLoadedState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  void* pWorkerRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);
}

void TilesetContentLoader::unloadDoneState(Tile& tile) {
  TileContent* pContent = tile.exp_GetContent();
  void* pMainThreadRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
}

void TilesetContentLoader::broadCastRequestHeaderChange(
    CesiumAsync::IAssetAccessor::THeader&& header) {
  _requestHeadersChangeListener(std::move(header));
}
} // namespace Cesium3DTilesSelection
