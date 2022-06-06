#include "TilesetContentManager.h"

#include "TilesetContentLoader.h"

#include <Cesium3DTilesSelection/RasterOverlay.h>
#include <Cesium3DTilesSelection/RasterOverlayTileProvider.h>
#include <Cesium3DTilesSelection/RasterOverlayTile.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/joinToString.h>

#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
struct TileLoadResultAndRenderResources {
  TileLoadResult result;
  void* pRenderResources{nullptr};
};

TileLoadResultAndRenderResources postProcessGltf(
    TileLoadResult&& result,
    const glm::dmat4& tileTransform,
    const TilesetContentOptions& contentOptions,
    const std::shared_ptr<IPrepareRendererResources>&
        pPrepareRendererResources) {
  TileRenderContent& renderContent =
      std::get<TileRenderContent>(result.contentKind);

  if (result.pCompletedRequest) {
    renderContent.model->extras["Cesium3DTiles_TileUrl"] =
        result.pCompletedRequest->url();
  }

  if (contentOptions.generateMissingNormalsSmooth) {
    renderContent.model->generateMissingNormalsSmooth();
  }

  void* pRenderResources = pPrepareRendererResources->prepareInLoadThread(
      *renderContent.model,
      tileTransform);

  return TileLoadResultAndRenderResources{std::move(result), pRenderResources};
}

CesiumAsync::Future<TileLoadResultAndRenderResources> postProcessContent(
    TileLoadResult&& result,
    CesiumAsync::AsyncSystem&& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
    std::shared_ptr<spdlog::logger>&& pLogger,
    std::shared_ptr<IPrepareRendererResources>&& pPrepareRendererResources,
    const TilesetContentOptions& contentOptions,
    const glm::dmat4& tileTransform) {
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
      gltfOptions.ktx2TranscodeTargets = contentOptions.ktx2TranscodeTargets;

      return CesiumGltfReader::GltfReader::resolveExternalData(
                 asyncSystem,
                 baseUrl,
                 requestHeaders,
                 pAssetAccessor,
                 gltfOptions,
                 std::move(gltfResult))
          .thenInWorkerThread(
              [pLogger,
               tileTransform,
               contentOptions,
               result = std::move(result),
               pPrepareRendererResources =
                   std::move(pPrepareRendererResources)](
                  CesiumGltfReader::GltfReaderResult&& gltfResult) mutable {
                if (!gltfResult.errors.empty()) {
                  if (result.pCompletedRequest) {
                    SPDLOG_LOGGER_ERROR(
                        pLogger,
                        "Failed resolving external glTF buffers from {}:\n- {}",
                        result.pCompletedRequest->url(),
                        CesiumUtility::joinToString(gltfResult.errors, "\n- "));
                  } else {
                    SPDLOG_LOGGER_ERROR(
                        pLogger,
                        "Failed resolving external glTF buffers:\n- {}",
                        CesiumUtility::joinToString(gltfResult.errors, "\n- "));
                  }
                }

                if (!gltfResult.warnings.empty()) {
                  if (result.pCompletedRequest) {
                    SPDLOG_LOGGER_WARN(
                        pLogger,
                        "Warning when resolving external gltf buffers from "
                        "{}:\n- {}",
                        result.pCompletedRequest->url(),
                        CesiumUtility::joinToString(gltfResult.errors, "\n- "));
                  } else {
                    SPDLOG_LOGGER_ERROR(
                        pLogger,
                        "Warning resolving external glTF buffers:\n- {}",
                        CesiumUtility::joinToString(gltfResult.errors, "\n- "));
                  }
                }

                TileRenderContent& renderContent =
                    std::get<TileRenderContent>(result.contentKind);
                renderContent.model = std::move(gltfResult.model);
                return postProcessGltf(
                    std::move(result),
                    tileTransform,
                    contentOptions,
                    pPrepareRendererResources);
              });
    }
  }

  return asyncSystem.createResolvedFuture(
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
  TileContent& content = tile.getContent();
  if (content.getState() != TileLoadState::Unloaded &&
      content.getState() != TileLoadState::FailedTemporarily) {
    return;
  }

  notifyTileStartLoading(tile);

  const glm::dmat4& tileTransform = tile.getTransform();

  content.setState(TileLoadState::ContentLoading);
  _pLoader
      ->loadTileContent(
          tile,
          contentOptions,
          _externals.asyncSystem,
          _externals.pAssetAccessor,
          _externals.pLogger,
          _requestHeaders)
      .thenInWorkerThread(
          [pPrepareRendererResources = _externals.pPrepareRendererResources,
           asyncSystem = _externals.asyncSystem,
           pAssetAccessor = _externals.pAssetAccessor,
           pLogger = _externals.pLogger,
           contentOptions,
           tileTransform](TileLoadResult&& result) mutable {
            return postProcessContent(
                std::move(result),
                std::move(asyncSystem),
                std::move(pAssetAccessor),
                std::move(pLogger),
                std::move(pPrepareRendererResources),
                contentOptions,
                tileTransform);
          })
      .thenInMainThread([&tile, this](TileLoadResultAndRenderResources&& pair) {
        TilesetContentManager::setTileContent(
            tile.getContent(),
            std::move(pair.result),
            pair.pRenderResources);

        notifyTileDoneLoading(tile);
      })
      .catchInMainThread(
          [pLogger = _externals.pLogger, &tile, this](std::exception&& e) {
            notifyTileDoneLoading(tile);
            SPDLOG_LOGGER_ERROR(
                pLogger,
                "An unexpected error occurs when loading tile: {}",
                e.what());
          });
}

void TilesetContentManager::updateTileContent(Tile& tile) {
  const TileContent& content = tile.getContent();

  TileLoadState state = content.getState();
  switch (state) {
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

bool TilesetContentManager::unloadTileContent(Tile& tile) {
  TileContent& content = tile.getContent();
  TileLoadState state = content.getState();
  if (state == TileLoadState::Unloaded) {
    return true;
  }

  if (state == TileLoadState::ContentLoading) {
    return false;
  }

  if (content.isExternalContent() || content.isEmptyContent()) {
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

  content.setContentKind(TileUnknownContent{});
  content.setTileInitializerCallback({});
  content.setState(TileLoadState::Unloaded);
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
  content.setTileInitializerCallback(std::move(result.tileInitializer));
  content.setRenderResources(pWorkerRenderResources);
  if (result.tileProjection) {
    content.setProjection(*result.tileProjection);
  }
}

void TilesetContentManager::updateContentLoadedState(Tile& tile) {
  // initialize this tile content first
  TileContent& content = tile.getContent();
  auto& tileInitializer = content.getTileInitializerCallback();
  if (tileInitializer) {
    tileInitializer(tile);
  }

  // if tile is external tileset, then it will be refined no matter what
  if (content.isExternalContent()) {
    tile.setUnconditionallyRefine();
  }

  // create render resources in the main thread
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent && pRenderContent->model) {
    void* pWorkerRenderResources = content.getRenderResources();
    void* pMainThreadRenderResources =
        _externals.pPrepareRendererResources->prepareInMainThread(
            tile,
            pWorkerRenderResources);
    content.setRenderResources(pMainThreadRenderResources);
  }

  content.setState(TileLoadState::Done);
}

void TilesetContentManager::updateDoneState(Tile& tile) {
  // update raster overlay
  const TileContent& content = tile.getContent();

  bool moreRasterDetailAvailable = false;
  bool skippedUnknown = false;
  std::vector<RasterMappedTo3DTile> rasterTiles;
  for (size_t i = 0; i < rasterTiles.size(); ++i) {
    RasterMappedTo3DTile& mappedRasterTile = rasterTiles[i];

    RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
    if (pLoadingTile &&
        pLoadingTile->getState() == RasterOverlayTile::LoadState::Placeholder) {
      RasterOverlayTileProvider* pProvider =
          pLoadingTile->getOverlay().getTileProvider();

      // Try to replace this placeholder with real tiles.
      if (pProvider && !pProvider->isPlaceholder()) {
        // Remove the existing placeholder mapping
        rasterTiles.erase(
            rasterTiles.begin() +
            static_cast<std::vector<RasterMappedTo3DTile>::difference_type>(i));
        --i;

        // Add a new mapping.
        std::vector<CesiumGeospatial::Projection> missingProjections;
        RasterMappedTo3DTile::mapOverlayToTile(
            pProvider->getOwner(),
            tile,
            missingProjections);

        if (!missingProjections.empty()) {
          // The mesh doesn't have the right texture coordinates for this
          // overlay's projection, so we need to kick it back to the unloaded
          // state to fix that.
          // In the future, we could add the ability to add the required
          // texture coordinates without starting over from scratch.
          unloadTileContent(tile);
          return;
        }
      }

      continue;
    }

    const RasterOverlayTile::MoreDetailAvailable moreDetailAvailable =
        mappedRasterTile.update(*_externals.pPrepareRendererResources, tile);

    if (moreDetailAvailable ==
            RasterOverlayTile::MoreDetailAvailable::Unknown &&
        !moreRasterDetailAvailable) {
      skippedUnknown = true;
    }

    moreRasterDetailAvailable |=
        moreDetailAvailable == RasterOverlayTile::MoreDetailAvailable::Yes;
  }

  // If this tile still has no children after it's done loading, but it does
  // have raster tiles that are not the most detailed available, create fake
  // children to hang more detailed rasters on by subdividing this tile.
  if (!skippedUnknown && moreRasterDetailAvailable &&
      tile.getChildren().empty()) {
    createQuadtreeSubdividedChildren(tile);
  }
}

void TilesetContentManager::unloadContentLoadedState(Tile& tile) {
  TileContent& content = tile.getContent();
  void* pWorkerRenderResources = content.getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);
  content.setRenderResources(nullptr);
}

void TilesetContentManager::unloadDoneState(Tile& tile) {
  TileContent& content = tile.getContent();
  void* pMainThreadRenderResources = content.getRenderResources();
  _externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
  content.setRenderResources(nullptr);
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
