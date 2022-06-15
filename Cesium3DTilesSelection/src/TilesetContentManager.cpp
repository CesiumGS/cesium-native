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

struct RegionAndCenter {
  CesiumGeospatial::BoundingRegion region;
  CesiumGeospatial::Cartographic center;
};

std::optional<RegionAndCenter>
getTileBoundingRegionForUpsampling(const Tile& parent) {
  // To create subdivided children, we need to know a bounding region for each.
  // If the parent is already loaded and we have Web Mercator or Geographic
  // textures coordinates, we're set. If it's not, but it has a bounding region,
  // we're still set. Otherwise, we can't upsample (yet?).

  // Get an accurate bounding region from the content first.
  const TileContent& parentContent = parent.getContent();
  const RasterOverlayDetails* pOverlayDetails =
      parentContent.getRasterOverlayDetails();
  if (pOverlayDetails) {
    const RasterOverlayDetails& details = *pOverlayDetails;

    // If we don't have any overlay projections/rectangles, why are we
    // upsampling?
    assert(!details.rasterOverlayProjections.empty());
    assert(!details.rasterOverlayRectangles.empty());

    // Use the projected center of the tile as the subdivision center.
    // The tile will be subdivided by (0.5, 0.5) in the first overlay's
    // texture coordinates which overlay had more detail.
    for (const RasterMappedTo3DTile& mapped : parent.getMappedRasterTiles()) {
      if (mapped.isMoreDetailAvailable()) {
        const CesiumGeospatial::Projection& projection = mapped.getReadyTile()
                                                             ->getOverlay()
                                                             .getTileProvider()
                                                             ->getProjection();
        glm::dvec2 centerProjected =
            details.findRectangleForOverlayProjection(projection)->getCenter();
        CesiumGeospatial::Cartographic center =
            CesiumGeospatial::unprojectPosition(
                projection,
                glm::dvec3(centerProjected, 0.0));

        return RegionAndCenter{details.boundingRegion, center};
      }
    }
  }

  // We shouldn't be upsampling from a tile until that tile is loaded.
  // If it has no content after loading, we can't upsample from it.
  return std::nullopt;
}

void createQuadtreeSubdividedChildren(Tile& parent) {
  std::optional<RegionAndCenter> maybeRegionAndCenter =
      getTileBoundingRegionForUpsampling(parent);
  if (!maybeRegionAndCenter) {
    return;
  }

  // The quadtree tile ID doesn't actually matter, because we're not going to
  // use the standard tile bounds for the ID. But having a tile ID that reflects
  // the level and _approximate_ location is helpful for debugging.
  const CesiumGeometry::QuadtreeTileID* pRealParentTileID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&parent.getTileID());
  if (!pRealParentTileID) {
    const CesiumGeometry::UpsampledQuadtreeNode* pUpsampledID =
        std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&parent.getTileID());
    if (pUpsampledID) {
      pRealParentTileID = &pUpsampledID->tileID;
    }
  }

  CesiumGeometry::QuadtreeTileID parentTileID =
      pRealParentTileID ? *pRealParentTileID
                        : CesiumGeometry::QuadtreeTileID(0, 0, 0);

  // QuadtreeTileID can't handle higher than level 30 because the x and y
  // coordinates (uint32_t) will overflow. So just start over at level 0.
  if (parentTileID.level >= 30U) {
    parentTileID = CesiumGeometry::QuadtreeTileID(0, 0, 0);
  }

  // The parent tile must not have a zero geometric error, even if it's a leaf
  // tile. Otherwise we'd never refine it.
  parent.setGeometricError(parent.getNonZeroGeometricError());

  // The parent must use REPLACE refinement.
  parent.setRefine(TileRefine::Replace);

  // add 4 children for parent
  std::vector<Tile> children;
  children.reserve(4);
  for (std::size_t i = 0; i < 4; ++i) {
    children.emplace_back(parent.getContent().getLoader());
  }
  parent.createChildTiles(std::move(children));

  // populate children metadata
  gsl::span<Tile> childrenView = parent.getChildren();
  Tile& sw = childrenView[0];
  Tile& se = childrenView[1];
  Tile& nw = childrenView[2];
  Tile& ne = childrenView[3];

  // set children geometric error
  const double geometricError = parent.getGeometricError() * 0.5;
  sw.setGeometricError(geometricError);
  se.setGeometricError(geometricError);
  nw.setGeometricError(geometricError);
  ne.setGeometricError(geometricError);

  // set children tile ID
  const CesiumGeometry::QuadtreeTileID swID(
      parentTileID.level + 1,
      parentTileID.x * 2,
      parentTileID.y * 2);
  const CesiumGeometry::QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const CesiumGeometry::QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const CesiumGeometry::QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  sw.setTileID(CesiumGeometry::UpsampledQuadtreeNode{swID});
  se.setTileID(CesiumGeometry::UpsampledQuadtreeNode{seID});
  nw.setTileID(CesiumGeometry::UpsampledQuadtreeNode{nwID});
  ne.setTileID(CesiumGeometry::UpsampledQuadtreeNode{neID});

  // set children bounding volume
  const double minimumHeight = maybeRegionAndCenter->region.getMinimumHeight();
  const double maximumHeight = maybeRegionAndCenter->region.getMaximumHeight();

  const CesiumGeospatial::GlobeRectangle& parentRectangle =
      maybeRegionAndCenter->region.getRectangle();
  const CesiumGeospatial::Cartographic& center = maybeRegionAndCenter->center;
  sw.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              parentRectangle.getWest(),
              parentRectangle.getSouth(),
              center.longitude,
              center.latitude),
          minimumHeight,
          maximumHeight)));

  se.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              parentRectangle.getSouth(),
              parentRectangle.getEast(),
              center.latitude),
          minimumHeight,
          maximumHeight)));

  nw.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              parentRectangle.getWest(),
              center.latitude,
              center.longitude,
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight)));

  ne.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              center.latitude,
              parentRectangle.getEast(),
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight)));

  // set children transforms
  sw.setTransform(parent.getTransform());
  se.setTransform(parent.getTransform());
  nw.setTransform(parent.getTransform());
  ne.setTransform(parent.getTransform());
}

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
  assert(
      result.state == TileLoadResultState::Success &&
      "This function requires result to be success");

  TileRenderContent* pRenderContent =
      std::get_if<TileRenderContent>(&result.contentKind);
  assert(
      (pRenderContent && pRenderContent->model != std::nullopt) &&
      "This function only processes render content");

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
           pPrepareRendererResources = std::move(pPrepareRendererResources)](
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
    const TilesetOptions& tilesetOptions) {
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
          tilesetOptions.contentOptions,
          _externals.asyncSystem,
          _externals.pAssetAccessor,
          _externals.pLogger,
          _requestHeaders)
      .thenImmediately(
          [pPrepareRendererResources = _externals.pPrepareRendererResources,
           asyncSystem = _externals.asyncSystem,
           pAssetAccessor = _externals.pAssetAccessor,
           pLogger = _externals.pLogger,
           contentOptions = tilesetOptions.contentOptions,
           tileTransform](TileLoadResult&& result) mutable {
            // the reason we run immediate continuation, instead of in the
            // worker thread, is that the loader may run the task in the main
            // thread. And most often than not, those main thread task is very
            // light weight. So when those tasks return, there is no need to
            // spawn another worker thread if the result of the task isn't
            // related to render content. We only ever spawn a new task in the
            // worker thread if the content is a render content
            if (result.state == TileLoadResultState::Success) {
              auto pRenderContent =
                  std::get_if<TileRenderContent>(&result.contentKind);
              if (pRenderContent && pRenderContent->model) {
                return asyncSystem.runInWorkerThread(
                    [result = std::move(result),
                     asyncSystem,
                     pAssetAccessor = std::move(pAssetAccessor),
                     pLogger = std::move(pLogger),
                     pPrepareRendererResources =
                         std::move(pPrepareRendererResources),
                     contentOptions,
                     tileTransform]() mutable {
                      return postProcessContent(
                          std::move(result),
                          std::move(asyncSystem),
                          std::move(pAssetAccessor),
                          std::move(pLogger),
                          std::move(pPrepareRendererResources),
                          contentOptions,
                          tileTransform);
                    });
              }
            }

            return asyncSystem
                .createResolvedFuture<TileLoadResultAndRenderResources>(
                    {std::move(result), nullptr});
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

void TilesetContentManager::updateTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  TileContent& content = tile.getContent();

  TileLoadState state = content.getState();
  switch (state) {
  case TileLoadState::ContentLoaded:
    updateContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    updateDoneState(tile, tilesetOptions);
    break;
  default:
    break;
  }

  if (content.shouldContentContinueUpdated()) {
    content.setContentShouldContinueUpdated(_pLoader->updateTileContent(tile));
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
  content.setRenderResources(pWorkerRenderResources);
  content.setTileInitializerCallback(std::move(result.tileInitializer));
}

void TilesetContentManager::updateContentLoadedState(Tile& tile) {
  // initialize this tile content first
  TileContent& content = tile.getContent();
  if (content.isExternalContent()) {
    // if tile is external tileset, then it will be refined no matter what
    tile.setUnconditionallyRefine();
  } else if (content.isRenderContent()) {
    // create render resources in the main thread
    const TileRenderContent* pRenderContent = content.getRenderContent();
    if (pRenderContent->model) {
      void* pWorkerRenderResources = content.getRenderResources();
      void* pMainThreadRenderResources =
          _externals.pPrepareRendererResources->prepareInMainThread(
              tile,
              pWorkerRenderResources);
      content.setRenderResources(pMainThreadRenderResources);
    }
  }

  // call the initializer
  auto& tileInitializer = content.getTileInitializerCallback();
  if (tileInitializer) {
    tileInitializer(tile);
    content.setTileInitializerCallback({});
  }

  content.setState(TileLoadState::Done);
}

void TilesetContentManager::updateDoneState(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  if (tile.getContent().shouldContentContinueUpdated()) {
    return;
  }

  // update raster overlay
  const TileContent& content = tile.getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent && pRenderContent->model) {
    bool moreRasterDetailAvailable = false;
    bool skippedUnknown = false;
    std::vector<RasterMappedTo3DTile>& rasterTiles =
        tile.getMappedRasterTiles();
    for (size_t i = 0; i < rasterTiles.size(); ++i) {
      RasterMappedTo3DTile& mappedRasterTile = rasterTiles[i];

      RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              RasterOverlayTile::LoadState::Placeholder) {
        RasterOverlayTileProvider* pProvider =
            pLoadingTile->getOverlay().getTileProvider();

        // Try to replace this placeholder with real tiles.
        if (pProvider && !pProvider->isPlaceholder()) {
          // Remove the existing placeholder mapping
          rasterTiles.erase(
              rasterTiles.begin() +
              static_cast<std::vector<RasterMappedTo3DTile>::difference_type>(
                  i));
          --i;

          // Add a new mapping.
          std::vector<CesiumGeospatial::Projection> missingProjections;
          RasterMappedTo3DTile::mapOverlayToTile(
              tilesetOptions.maximumScreenSpaceError,
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
  assert(
      _tilesLoadOnProgress > 0 &&
      "There are no tile loads currently in flight");
  --_tilesLoadOnProgress;
  _tilesDataUsed += tile.computeByteSize();
}

void TilesetContentManager::notifyTileUnloading(Tile& tile) noexcept {
  _tilesDataUsed -= tile.computeByteSize();
}
} // namespace Cesium3DTilesSelection
