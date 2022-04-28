#include <Cesium3DTilesSelection/Exp_TileContentLoadInfo.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentManager.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <CesiumGltfReader/GltfReader.h>

namespace Cesium3DTilesSelection {
namespace {
struct TileLoadResultAndRenderResources {
  TileLoadResult result;
  void* pRenderResources{nullptr};
};

TileLoadResultAndRenderResources postProcessGltf(
    const TileContentLoadInfo& loadInfo,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    TileLoadResult&& result) {
  TileRenderContent& renderContent =
      std::get<TileRenderContent>(result.contentKind);
  const TilesetContentOptions& tilesetOptions = loadInfo.contentOptions;
  if (tilesetOptions.generateMissingNormalsSmooth) {
    renderContent.model->generateMissingNormalsSmooth();
  }

  void* pRenderResources = pPrepareRendererResources->prepareInLoadThread(
      *renderContent.model,
      loadInfo.tileTransform);

  return TileLoadResultAndRenderResources{std::move(result), pRenderResources};
}

CesiumAsync::Future<TileLoadResultAndRenderResources> postProcessContent(
    const TileContentLoadInfo& loadInfo,
    TileLoadResult&& result,
    std::shared_ptr<IPrepareRendererResources>&& pPrepareRendererResources) {
  void* pRenderResources = nullptr;
  if (result.state == TileLoadResultState::Success) {
    TileRenderContent* pRenderContent =
        std::get_if<TileRenderContent>(&result.contentKind);
    if (pRenderContent && pRenderContent->model) {
      // Download any external image or buffer urls in the gltf if there are any
      CesiumGltfReader::GltfReaderResult gltfResult{
          std::move(*pRenderContent->model),
          {},
          {}};

      CesiumAsync::HttpHeaders requestHeaders;
      std::string baseUrl;
      if (result.pCompletedRequest) {
        requestHeaders = result.pCompletedRequest->headers();
        baseUrl = result.pCompletedRequest->url();
      }

      CesiumGltfReader::GltfReaderOptions gltfOptions;
      gltfOptions.ktx2TranscodeTargets =
          loadInfo.contentOptions.ktx2TranscodeTargets;

      return CesiumGltfReader::GltfReader::resolveExternalData(
                 loadInfo.asyncSystem,
                 baseUrl,
                 requestHeaders,
                 loadInfo.pAssetAccessor,
                 gltfOptions,
                 std::move(gltfResult))
          .thenInWorkerThread(
              [loadInfo,
               result = std::move(result),
               pPrepareRendererResources =
                   std::move(pPrepareRendererResources)](
                  CesiumGltfReader::GltfReaderResult&& gltfResult) mutable {
                TileRenderContent& renderContent =
                    std::get<TileRenderContent>(result.contentKind);
                renderContent.model = std::move(gltfResult.model);
                return postProcessGltf(
                    loadInfo,
                    pPrepareRendererResources,
                    std::move(result));
              });
    }
  }

  return loadInfo.asyncSystem.createResolvedFuture(
      TileLoadResultAndRenderResources{std::move(result), pRenderResources});
}
} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    std::unique_ptr<TilesetContentLoader> pLoader)
    : _externals{externals},
      _requestHeaders{std::move(requestHeaders)},
      _pLoader{std::move(pLoader)},
      _tilesLoadOnProgress{0},
      _tilesDataUsed{0} {}

TilesetContentManager::~TilesetContentManager() noexcept {
  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _tilesLoadOnProgress not
  // being decremented correctly when an async load ends.
  while (_tilesLoadOnProgress > 0) {
    _externals.pAssetAccessor->tick();
    _externals.asyncSystem.dispatchMainThreadTasks();
  }
}

void TilesetContentManager::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions) {
  TileContent* pContent = tile.getContent();
  if (!pContent) {
    return;
  }

  if (pContent->getState() != TileLoadState::Unloaded &&
      pContent->getState() != TileLoadState::FailedTemporarily) {
    return;
  }

  notifyTileStartLoading(tile);

  TileContentLoadInfo loadInfo{
      _externals.asyncSystem,
      _externals.pAssetAccessor,
      _externals.pLogger,
      contentOptions,
      tile};

  pContent->setState(TileLoadState::ContentLoading);
  _pLoader->loadTileContent(*pContent->getLoader(), loadInfo, _requestHeaders)
      .thenInWorkerThread(
          [pPrepareRendererResources = _externals.pPrepareRendererResources,
           loadInfo](TileLoadResult&& result) mutable {
            return postProcessContent(
                loadInfo,
                std::move(result),
                std::move(pPrepareRendererResources));
          })
      .thenInMainThread([&tile, this](TileLoadResultAndRenderResources&& pair) {
        TilesetContentManager::setTileContent(
            *tile.getContent(),
            std::move(pair.result),
            pair.pRenderResources);

        notifyTileDoneLoading(tile);
      });
}

void TilesetContentManager::updateTileContent(Tile& tile) {
  const TileContent* pContent = tile.getContent();
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

bool TilesetContentManager::unloadTileContent(Tile& tile) {
  TileContent* pContent = tile.getContent();
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

  if (pContent->isExternalContent()) {
    return false;
  }

  notifyTileUnloading(tile);

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

  pContent->setContentKind(TileUnknownContent{});
  pContent->setTileInitializerCallback({});
  pContent->setState(TileLoadState::Unloaded);
  return true;
}

const std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetContentManager::getRequestHeaders() const noexcept {
  return _requestHeaders;
}

std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetContentManager::getRequestHeaders() noexcept {
  return _requestHeaders;
}

int32_t TilesetContentManager::getNumOfTilesLoading() const noexcept {
  return _tilesLoadOnProgress;
}

int64_t TilesetContentManager::getTilesDataUsed() const noexcept {
  return _tilesDataUsed;
}

void TilesetContentManager::setTileContent(
    TileContent& content,
    TileLoadResult&& result,
    void* pWorkerRenderResources) {
  switch (result.state) {
  case TileLoadResultState::Success:
    content.setState(TileLoadState::ContentLoaded);
    break;
  case TileLoadResultState::Failed:
    content.setState(TileLoadState::Failed);
    break;
  case TileLoadResultState::RetryLater:
    content.setState(TileLoadState::FailedTemporarily);
    break;
  default:
    assert(false && "Cannot handle an unknown TileLoadResultState");
    break;
  }

  content.setContentKind(std::move(result.contentKind));
  content.setTileInitializerCallback(std::move(result.deferredTileInitializer));
  content.setRenderResources(pWorkerRenderResources);
}

void TilesetContentManager::updateContentLoadedState(Tile& tile) {
  // initialize this tile content first
  TileContent* pContent = tile.getContent();
  auto& tileInitializer = pContent->getTileInitializerCallback();
  if (tileInitializer) {
    tileInitializer(tile);
  }

  // if tile is external tileset, then it will be refined no matter what
  if (pContent->isExternalContent()) {
    tile.setUnconditionallyRefine();
  }

  // create render resources in the main thread
  const TileRenderContent* pRenderContent = pContent->getRenderContent();
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

void TilesetContentManager::unloadContentLoadedState(Tile& tile) {
  TileContent* pContent = tile.getContent();
  void* pWorkerRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);
  pContent->setRenderResources(nullptr);
}

void TilesetContentManager::unloadDoneState(Tile& tile) {
  TileContent* pContent = tile.getContent();
  void* pMainThreadRenderResources = pContent->getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
  pContent->setRenderResources(nullptr);
}

void TilesetContentManager::notifyTileStartLoading(
    [[maybe_unused]] Tile& tile) noexcept {
  ++_tilesLoadOnProgress;
}

void TilesetContentManager::notifyTileDoneLoading(Tile& tile) noexcept {
  assert(_tilesLoadOnProgress > 0 && "There are no tiles currently on the fly");
  --_tilesLoadOnProgress;
  _tilesDataUsed += tile.computeByteSize();
}

void TilesetContentManager::notifyTileUnloading(Tile& tile) noexcept {
  _tilesDataUsed -= tile.computeByteSize();
}
} // namespace Cesium3DTilesSelection
