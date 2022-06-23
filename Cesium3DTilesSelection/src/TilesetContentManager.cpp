#include "TilesetContentManager.h"

#include "TileContentLoadInfo.h"
#include "TilesetContentLoader.h"

#include <Cesium3DTilesSelection/GltfUtilities.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/RasterOverlay.h>
#include <Cesium3DTilesSelection/RasterOverlayTile.h>
#include <Cesium3DTilesSelection/RasterOverlayTileProvider.h>
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

bool anyRasterOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    const RasterOverlayTile* pLoading = mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() == RasterOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

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

void createQuadtreeSubdividedChildren(Tile& parent, RasterOverlayUpsampler &upsampler) {
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
    children.emplace_back(&upsampler);
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

std::vector<CesiumGeospatial::Projection> mapOverlaysToTile(
    Tile& tile,
    RasterOverlayCollection& overlays,
    const TilesetOptions& tilesetOptions) {
  // when tile fails temporarily, it may still have mapped raster tiles, so
  // clear it here
  tile.getMappedRasterTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;
  for (auto& pOverlay : overlays) {
    RasterMappedTo3DTile* pMapped = RasterMappedTo3DTile::mapOverlayToTile(
        tilesetOptions.maximumScreenSpaceError,
        *pOverlay,
        tile,
        projections);
    if (pMapped) {
      // Try to load now, but if the mapped raster tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
}

const BoundingVolume& getEffectiveBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    [[maybe_unused]] const std::optional<BoundingVolume>&
        updatedTileContentBoundingVolume) {
  // If we have an updated tile bounding volume, use it.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // If we _only_ have an updated _content_ bounding volume, that's a developer
  // error.
  assert(!updatedTileContentBoundingVolume);

  return tileBoundingVolume;
}

const BoundingVolume& getEffectiveContentBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& tileContentBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileContentBoundingVolume) {
  // If we have an updated tile content bounding volume, use it.
  if (updatedTileContentBoundingVolume) {
    return *updatedTileContentBoundingVolume;
  }

  // Next best thing is an updated tile non-content bounding volume.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // Then a content bounding volume attached to the tile.
  if (tileContentBoundingVolume) {
    return *tileContentBoundingVolume;
  }

  // And finally the regular tile bounding volume.
  return tileBoundingVolume;
}

TileLoadResultAndRenderResources postProcessGltfInWorkerThread(
    TileLoadResult&& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  TileRenderContent& renderContent =
      std::get<TileRenderContent>(result.contentKind);

  if (result.pCompletedRequest) {
    renderContent.model->extras["Cesium3DTiles_TileUrl"] =
        result.pCompletedRequest->url();
  }

  const BoundingVolume& contentBoundingVolume =
      getEffectiveContentBoundingVolume(
          tileLoadInfo.tileBoundingVolume,
          tileLoadInfo.tileContentBoundingVolume,
          result.updatedBoundingVolume,
          result.updatedContentBoundingVolume);

  // If we have projections, generate texture coordinates for all of them. Also
  // remember the min and max height so that we can use them for upsampling.
  const CesiumGeospatial::BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(contentBoundingVolume);
  result.overlayDetails = GltfUtilities::createRasterOverlayTextureCoordinates(
      *renderContent.model,
      tileLoadInfo.tileTransform,
      0,
      pRegion ? std::make_optional(pRegion->getRectangle()) : std::nullopt,
      std::move(projections));

  if (pRegion && result.overlayDetails) {
    // If the original bounding region was wrong, report it.
    const CesiumGeospatial::GlobeRectangle& original = pRegion->getRectangle();
    const CesiumGeospatial::GlobeRectangle& computed =
        result.overlayDetails->boundingRegion.getRectangle();
    if ((!CesiumUtility::Math::equalsEpsilon(
             computed.getWest(),
             original.getWest(),
             0.01) &&
         computed.getWest() < original.getWest()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getSouth(),
             original.getSouth(),
             0.01) &&
         computed.getSouth() < original.getSouth()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getEast(),
             original.getEast(),
             0.01) &&
         computed.getEast() > original.getEast()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getNorth(),
             original.getNorth(),
             0.01) &&
         computed.getNorth() > original.getNorth())) {

      auto it = renderContent.model->extras.find("Cesium3DTiles_TileUrl");
      std::string url = it != renderContent.model->extras.end()
                            ? it->second.getStringOrDefault("Unknown Tile URL")
                            : "Unknown Tile URL";
      SPDLOG_LOGGER_WARN(
          tileLoadInfo.pLogger,
          "Tile has a bounding volume that does not include all of its "
          "content, so culling and raster overlays may be incorrect: {}",
          url);
    }
  }

  // If our tile bounding region has loose fitting heights, find the real ones.
  const BoundingVolume& boundingVolume = getEffectiveBoundingVolume(
      tileLoadInfo.tileBoundingVolume,
      result.updatedBoundingVolume,
      result.updatedContentBoundingVolume);
  if (std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(
          &boundingVolume) != nullptr) {
    if (result.overlayDetails) {
      // We already computed the bounding region for overlays, so use it.
      result.updatedBoundingVolume = result.overlayDetails->boundingRegion;
    } else {
      // We need to compute an accurate bounding region
      result.updatedBoundingVolume = GltfUtilities::computeBoundingRegion(
          *renderContent.model,
          tileLoadInfo.tileTransform);
    }
  }

  // generate missing smooth normal
  if (tileLoadInfo.contentOptions.generateMissingNormalsSmooth) {
    renderContent.model->generateMissingNormalsSmooth();
  }

  void* pRenderResources =
      tileLoadInfo.pPrepareRendererResources->prepareInLoadThread(
          *renderContent.model,
          tileLoadInfo.tileTransform);

  return TileLoadResultAndRenderResources{std::move(result), pRenderResources};
}

CesiumAsync::Future<TileLoadResultAndRenderResources>
postProcessContentInWorkerThread(
    TileLoadResult&& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    TileContentLoadInfo&& tileLoadInfo) {
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
  gltfOptions.ktx2TranscodeTargets =
      tileLoadInfo.contentOptions.ktx2TranscodeTargets;

  auto asyncSystem = tileLoadInfo.asyncSystem;
  auto pAssetAccessor = tileLoadInfo.pAssetAccessor;
  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             baseUrl,
             requestHeaders,
             pAssetAccessor,
             gltfOptions,
             std::move(gltfResult))
      .thenInWorkerThread(
          [result = std::move(result),
           projections = std::move(projections),
           tileLoadInfo = std::move(tileLoadInfo)](
              CesiumGltfReader::GltfReaderResult&& gltfResult) mutable {
            if (!gltfResult.errors.empty()) {
              if (result.pCompletedRequest) {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers from {}:\n- {}",
                    result.pCompletedRequest->url(),
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            if (!gltfResult.warnings.empty()) {
              if (result.pCompletedRequest) {
                SPDLOG_LOGGER_WARN(
                    tileLoadInfo.pLogger,
                    "Warning when resolving external gltf buffers from "
                    "{}:\n- {}",
                    result.pCompletedRequest->url(),
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Warning resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            TileRenderContent& renderContent =
                std::get<TileRenderContent>(result.contentKind);
            renderContent.model = std::move(gltfResult.model);
            return postProcessGltfInWorkerThread(
                std::move(result),
                std::move(projections),
                tileLoadInfo);
          });
}
} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    RasterOverlayCollection& overlayCollection)
    : _externals{externals},
      _requestHeaders{std::move(requestHeaders)},
      _pLoader{std::move(pLoader)},
      _pOverlayCollection{&overlayCollection},
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

  // Wait for all overlays to wrap up their loading, too.
  uint32_t tilesLoading = 1;
  while (tilesLoading > 0) {
    _externals.pAssetAccessor->tick();
    _externals.asyncSystem.dispatchMainThreadTasks();

    tilesLoading = 0;
    for (const auto& pOverlay : *_pOverlayCollection) {
      tilesLoading += pOverlay->getTileProvider()->getNumberOfTilesLoading();
    }
  }
}

void TilesetContentManager::loadTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  TileContent& content = tile.getContent();
  if (content.getState() != TileLoadState::Unloaded &&
      content.getState() != TileLoadState::FailedTemporarily) {
    // No need to load geometry, but give previously-throttled
    // raster overlay tiles a chance to load.
    for (RasterMappedTo3DTile& rasterTile : tile.getMappedRasterTiles()) {
      rasterTile.loadThrottled();
    }

    return;
  }

  // Below are the guarantees the loader can assume about upsampled tile. If any
  // of those guarantees are wrong, it's a bug:
  // - Any tile that is marked as upsampled tile, we will guarantee that the
  // parent is always loaded. It lets the loader takes care of upsampling only
  // without requesting the parent tile. If a loader tries to upsample tile, but
  // the parent is not loaded, it is a bug.
  // - This manager will also guarantee that the parent tile will be alive until
  // the upsampled tile content returns to the main thread. So the loader can
  // capture the parent geometry by reference in the worker thread to upsample
  // the current tile. Warning: it's not thread-safe to modify the parent
  // geometry in the worker thread at the same time though
  const CesiumGeometry::UpsampledQuadtreeNode* pUpsampleID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&tile.getTileID());
  if (pUpsampleID) {
    // We can't upsample this tile until its parent tile is done loading.
    Tile* pParentTile = tile.getParent();
    if (pParentTile) {
      if (pParentTile->getState() != TileLoadState::Done) {
        loadTileContent(*pParentTile, tilesetOptions);
        return;
      }
    } else {
      // we cannot upsample this tile if it doesn't have parent
      return;
    }
  }

  // map raster overlay to tile
  std::vector<CesiumGeospatial::Projection> projections =
      mapOverlaysToTile(tile, *_pOverlayCollection, tilesetOptions);

  // begin loading tile
  notifyTileStartLoading(tile);
  content.setState(TileLoadState::ContentLoading);

  TileContentLoadInfo tileLoadInfo{
      _externals.asyncSystem,
      _externals.pAssetAccessor,
      _externals.pPrepareRendererResources,
      _externals.pLogger,
      tilesetOptions.contentOptions,
      tile};

  _pLoader
      ->loadTileContent(
          tile,
          tilesetOptions.contentOptions,
          _externals.asyncSystem,
          _externals.pAssetAccessor,
          _externals.pLogger,
          _requestHeaders)
      .thenImmediately([tileLoadInfo = std::move(tileLoadInfo),
                        projections = std::move(projections)](
                           TileLoadResult&& result) mutable {
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
            auto asyncSystem = tileLoadInfo.asyncSystem;
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 projections = std::move(projections),
                 tileLoadInfo = std::move(tileLoadInfo)]() mutable {
                  return postProcessContentInWorkerThread(
                      std::move(result),
                      std::move(projections),
                      std::move(tileLoadInfo));
                });
          }
        }

        return tileLoadInfo.asyncSystem
            .createResolvedFuture<TileLoadResultAndRenderResources>(
                {std::move(result), nullptr});
      })
      .thenInMainThread([&tile, this](TileLoadResultAndRenderResources&& pair) {
        TilesetContentManager::setTileContent(
            tile,
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

  // don't unload external or empty tile
  if (content.isExternalContent() || content.isEmptyContent()) {
    return false;
  }

  // don't unload tile if any of its children are upsampled tile and
  // currently loading. Loader can safely capture the parent tile to upsample
  // the current tile as the manager guarantee that the parent tile will be
  // alive
  for (const Tile& child : tile.getChildren()) {
    if (child.getState() == TileLoadState::ContentLoading &&
        std::holds_alternative<CesiumGeometry::UpsampledQuadtreeNode>(
            child.getTileID())) {
      return false;
    }
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

  tile.getMappedRasterTiles().clear();
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

int64_t TilesetContentManager::getTotalDataUsed() const noexcept {
  int64_t bytes = _tilesDataUsed;
  for (const auto& pOverlay : *_pOverlayCollection) {
    const RasterOverlayTileProvider* pProvider = pOverlay->getTileProvider();
    if (pProvider) {
      bytes += pProvider->getTileDataBytes();
    }
  }

  return bytes;
}

bool TilesetContentManager::doesTileNeedLoading(
    const Tile& tile) const noexcept {
  auto state = tile.getState();
  return state == TileLoadState::Unloaded ||
         state == TileLoadState::FailedTemporarily ||
         anyRasterOverlaysNeedLoading(tile);
}

void TilesetContentManager::setTileContent(
    Tile& tile,
    TileLoadResult&& result,
    void* pWorkerRenderResources) {
  // update bounding volume
  if (result.updatedBoundingVolume) {
    tile.setBoundingVolume(*result.updatedBoundingVolume);
  }

  if (result.updatedContentBoundingVolume) {
    tile.setContentBoundingVolume(*result.updatedContentBoundingVolume);
  }

  // set content
  auto& content = tile.getContent();
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
  if (result.overlayDetails) {
    content.setRasterOverlayDetails(std::move(*result.overlayDetails));
  }

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
      createQuadtreeSubdividedChildren(tile, _upsampler);
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
