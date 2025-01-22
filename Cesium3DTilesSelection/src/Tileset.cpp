#include "TilesetContentManager.h"
#include "TilesetHeightQuery.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/ITileExcluder.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Tracing.h>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

Tileset::Tileset(
    const TilesetExternals& externals,
    std::unique_ptr<TilesetContentLoader>&& pCustomLoader,
    std::unique_ptr<Tile>&& pRootTile,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _options(options),
      _previousFrameNumber(0),
      _distances(),
      _childOcclusionProxies(),
      _pTilesetContentManager{
          new TilesetContentManager(
              _externals,
              _options,
              RasterOverlayCollection{
                  _loadedTiles,
                  externals,
                  options.ellipsoid},
              std::move(pCustomLoader),
              std::move(pRootTile)),
      } {}

Tileset::Tileset(
    const TilesetExternals& externals,
    const std::string& url,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _options(options),
      _previousFrameNumber(0),
      _distances(),
      _childOcclusionProxies(),
      _pTilesetContentManager{
          new TilesetContentManager(
              _externals,
              _options,
              RasterOverlayCollection{
                  _loadedTiles,
                  externals,
                  options.ellipsoid},
              url),
      } {}

Tileset::Tileset(
    const TilesetExternals& externals,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const TilesetOptions& options,
    const std::string& ionAssetEndpointUrl)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _options(options),
      _previousFrameNumber(0),
      _distances(),
      _childOcclusionProxies(),
      _pTilesetContentManager{new TilesetContentManager(
          _externals,
          _options,
          RasterOverlayCollection{_loadedTiles, externals, options.ellipsoid},
          ionAssetID,
          ionAccessToken,
          ionAssetEndpointUrl)} {}

Tileset::~Tileset() noexcept {
  TilesetHeightRequest::failHeightRequests(
      this->_heightRequests,
      "Tileset is being destroyed.");

  this->_pTilesetContentManager->unloadAll();
  if (this->_externals.pTileOcclusionProxyPool) {
    this->_externals.pTileOcclusionProxyPool->destroyPool();
  }
}

CesiumAsync::SharedFuture<void>& Tileset::getAsyncDestructionCompleteEvent() {
  return this->_pTilesetContentManager->getAsyncDestructionCompleteEvent();
}

CesiumAsync::SharedFuture<void>& Tileset::getRootTileAvailableEvent() {
  return this->_pTilesetContentManager->getRootTileAvailableEvent();
}

const std::vector<Credit>& Tileset::getTilesetCredits() const noexcept {
  return this->_pTilesetContentManager->getTilesetCredits();
}

void Tileset::setShowCreditsOnScreen(bool showCreditsOnScreen) noexcept {
  this->_options.showCreditsOnScreen = showCreditsOnScreen;

  const std::vector<Credit>& credits = this->getTilesetCredits();
  auto pCreditSystem = this->_externals.pCreditSystem;
  for (auto credit : credits) {
    pCreditSystem->setShowOnScreen(credit, showCreditsOnScreen);
  }
}

Tile* Tileset::getRootTile() noexcept {
  return this->_pTilesetContentManager->getRootTile();
}

const Tile* Tileset::getRootTile() const noexcept {
  return this->_pTilesetContentManager->getRootTile();
}

RasterOverlayCollection& Tileset::getOverlays() noexcept {
  return this->_pTilesetContentManager->getRasterOverlayCollection();
}

const RasterOverlayCollection& Tileset::getOverlays() const noexcept {
  return this->_pTilesetContentManager->getRasterOverlayCollection();
}

TilesetSharedAssetSystem& Tileset::getSharedAssetSystem() noexcept {
  return *this->_pTilesetContentManager->getSharedAssetSystem();
}

const TilesetSharedAssetSystem& Tileset::getSharedAssetSystem() const noexcept {
  return *this->_pTilesetContentManager->getSharedAssetSystem();
}

// NOLINTBEGIN(misc-use-anonymous-namespace)
static bool
operator<(const FogDensityAtHeight& fogDensity, double height) noexcept {
  return fogDensity.cameraHeight < height;
}
// NOLINTEND(misc-use-anonymous-namespace)

namespace {

double computeFogDensity(
    const std::vector<FogDensityAtHeight>& fogDensityTable,
    const ViewState& viewState) {
  const double height = viewState.getPositionCartographic()
                            .value_or(Cartographic(0.0, 0.0, 0.0))
                            .height;

  // Find the entry that is for >= this camera height.
  auto nextIt =
      std::lower_bound(fogDensityTable.begin(), fogDensityTable.end(), height);

  if (nextIt == fogDensityTable.end()) {
    return fogDensityTable.back().fogDensity;
  }
  if (nextIt == fogDensityTable.begin()) {
    return nextIt->fogDensity;
  }

  auto prevIt = nextIt - 1;

  const double heightA = prevIt->cameraHeight;
  const double densityA = prevIt->fogDensity;

  const double heightB = nextIt->cameraHeight;
  const double densityB = nextIt->fogDensity;

  const double t =
      glm::clamp((height - heightA) / (heightB - heightA), 0.0, 1.0);

  const double density = glm::mix(densityA, densityB, t);

  // CesiumJS will also fade out the fog based on the camera angle,
  // so when we're looking straight down there's no fog. This is unfortunate
  // because it prevents the fog culling from being used in place of horizon
  // culling. Horizon culling is the only thing in CesiumJS that prevents
  // tiles on the back side of the globe from being rendered.
  // Since we're not actually _rendering_ the fog in cesium-native (that's on
  // the renderer), we don't need to worry about the fog making the globe
  // looked washed out in straight down views. So here we don't fade by
  // angle at all.

  return density;
}

} // namespace

void Tileset::_updateLodTransitions(
    const FrameState& frameState,
    float deltaTime,
    ViewUpdateResult& result) const noexcept {
  if (_options.enableLodTransitionPeriod) {
    // We always fade tiles from 0.0 --> 1.0. Whether the tile is fading in or
    // out is determined by whether the tile is in the tilesToRenderThisFrame
    // or tilesFadingOut list.
    float deltaTransitionPercentage =
        deltaTime / this->_options.lodTransitionLength;

    // Update fade out
    for (auto tileIt = result.tilesFadingOut.begin();
         tileIt != result.tilesFadingOut.end();) {
      TileRenderContent* pRenderContent =
          (*tileIt)->getContent().getRenderContent();

      if (!pRenderContent) {
        // This tile is done fading out and was immediately kicked from the
        // cache.
        tileIt = result.tilesFadingOut.erase(tileIt);
        continue;
      }

      // Remove tile from fade-out list if it is back on the render list.
      TileSelectionState::Result selectionResult =
          (*tileIt)->getLastSelectionState().getResult(
              frameState.currentFrameNumber);
      if (selectionResult == TileSelectionState::Result::Rendered) {
        // This tile will already be on the render list.
        pRenderContent->setLodTransitionFadePercentage(0.0f);
        tileIt = result.tilesFadingOut.erase(tileIt);
        continue;
      }

      float currentPercentage =
          pRenderContent->getLodTransitionFadePercentage();
      if (currentPercentage >= 1.0f) {
        // Remove this tile from the fading out list if it is already done.
        // The client will already have had a chance to stop rendering the tile
        // last frame.
        pRenderContent->setLodTransitionFadePercentage(0.0f);
        tileIt = result.tilesFadingOut.erase(tileIt);
        continue;
      }

      float newPercentage =
          glm::min(currentPercentage + deltaTransitionPercentage, 1.0f);
      pRenderContent->setLodTransitionFadePercentage(newPercentage);
      ++tileIt;
    }

    // Update fade in
    for (Tile* pTile : result.tilesToRenderThisFrame) {
      TileRenderContent* pRenderContent =
          pTile->getContent().getRenderContent();
      if (pRenderContent) {
        float transitionPercentage =
            pRenderContent->getLodTransitionFadePercentage();
        float newTransitionPercentage =
            glm::min(transitionPercentage + deltaTransitionPercentage, 1.0f);
        pRenderContent->setLodTransitionFadePercentage(newTransitionPercentage);
      }
    }
  } else {
    // If there are any tiles still fading in, set them to fully visible right
    // away.
    for (Tile* pTile : result.tilesToRenderThisFrame) {
      TileRenderContent* pRenderContent =
          pTile->getContent().getRenderContent();
      if (pRenderContent) {
        pRenderContent->setLodTransitionFadePercentage(1.0f);
      }
    }
  }
}

const ViewUpdateResult&
Tileset::updateViewOffline(const std::vector<ViewState>& frustums) {
  std::vector<Tile*> tilesSelectedPrevFrame =
      this->_updateResult.tilesToRenderThisFrame;

  // TODO: fix the fading for offline case
  // (https://github.com/CesiumGS/cesium-native/issues/549)
  this->updateView(frustums, 0.0f);
  while (this->_pTilesetContentManager->getNumberOfTilesLoading() > 0 ||
         this->_updateResult.mainThreadTileLoadQueueLength > 0 ||
         this->_updateResult.workerThreadTileLoadQueueLength > 0) {
    this->_externals.pAssetAccessor->tick();
    this->updateView(frustums, 0.0f);
  }

  this->_updateResult.tilesFadingOut.clear();

  std::unordered_set<Tile*> uniqueTilesToRenderThisFrame(
      this->_updateResult.tilesToRenderThisFrame.begin(),
      this->_updateResult.tilesToRenderThisFrame.end());
  for (Tile* tile : tilesSelectedPrevFrame) {
    if (uniqueTilesToRenderThisFrame.find(tile) ==
        uniqueTilesToRenderThisFrame.end()) {
      TileRenderContent* pRenderContent = tile->getContent().getRenderContent();
      if (pRenderContent) {
        pRenderContent->setLodTransitionFadePercentage(1.0f);
        this->_updateResult.tilesFadingOut.insert(tile);
      }
    }
  }

  return this->_updateResult;
}

const ViewUpdateResult&
Tileset::updateView(const std::vector<ViewState>& frustums, float deltaTime) {
  CESIUM_TRACE("Tileset::updateView");
  // Fixup TilesetOptions to ensure lod transitions works correctly.
  _options.enableFrustumCulling =
      _options.enableFrustumCulling && !_options.enableLodTransitionPeriod;
  _options.enableFogCulling =
      _options.enableFogCulling && !_options.enableLodTransitionPeriod;

  this->_asyncSystem.dispatchMainThreadTasks();

  const int32_t previousFrameNumber = this->_previousFrameNumber;
  const int32_t currentFrameNumber = previousFrameNumber + 1;

  ViewUpdateResult& result = this->_updateResult;
  result.frameNumber = currentFrameNumber;
  result.tilesToRenderThisFrame.clear();
  result.tilesVisited = 0;
  result.culledTilesVisited = 0;
  result.tilesCulled = 0;
  result.tilesOccluded = 0;
  result.tilesWaitingForOcclusionResults = 0;
  result.tilesKicked = 0;
  result.maxDepthVisited = 0;

  if (!_options.enableLodTransitionPeriod) {
    result.tilesFadingOut.clear();
  }

  Tile* pRootTile = this->getRootTile();
  if (!pRootTile) {
    // If the root tile is marked as ready, but doesn't actually exist, then
    // the tileset couldn't load. Fail any outstanding height requests.
    if (!this->_heightRequests.empty() && this->_pTilesetContentManager &&
        this->_pTilesetContentManager->getRootTileAvailableEvent().isReady()) {
      TilesetHeightRequest::failHeightRequests(
          this->_heightRequests,
          "Height requests could not complete because the tileset failed to "
          "load.");
    }
    return result;
  }

  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       this->_options.excluders) {
    pExcluder->startNewFrame();
  }

  this->_workerThreadLoadQueue.clear();
  this->_mainThreadLoadQueue.clear();

  std::vector<double> fogDensities(frustums.size());
  std::transform(
      frustums.begin(),
      frustums.end(),
      fogDensities.begin(),
      [&fogDensityTable =
           this->_options.fogDensityTable](const ViewState& frustum) -> double {
        return computeFogDensity(fogDensityTable, frustum);
      });

  FrameState frameState{
      frustums,
      std::move(fogDensities),
      previousFrameNumber,
      currentFrameNumber};

  if (!frustums.empty()) {
    this->_visitTileIfNeeded(frameState, 0, false, *pRootTile, result);
  } else {
    result = ViewUpdateResult();
  }

  TilesetHeightRequest::processHeightRequests(
      this->getAsyncSystem(),
      *this->_pTilesetContentManager,
      this->_options,
      this->_loadedTiles,
      this->_heightRequests,
      this->_heightQueryLoadQueue);

  result.workerThreadTileLoadQueueLength =
      static_cast<int32_t>(this->_workerThreadLoadQueue.size());
  result.mainThreadTileLoadQueueLength =
      static_cast<int32_t>(this->_mainThreadLoadQueue.size());

  const std::shared_ptr<TileOcclusionRendererProxyPool>& pOcclusionPool =
      this->_externals.pTileOcclusionProxyPool;
  if (pOcclusionPool) {
    pOcclusionPool->pruneOcclusionProxyMappings();
  }

  this->_unloadCachedTiles(this->_options.tileCacheUnloadTimeLimit);
  this->_processWorkerThreadLoadQueue();
  this->_processMainThreadLoadQueue();
  this->_updateLodTransitions(frameState, deltaTime, result);

  // aggregate all the credits needed from this tileset for the current frame
  const std::shared_ptr<CreditSystem>& pCreditSystem =
      this->_externals.pCreditSystem;
  if (pCreditSystem && !result.tilesToRenderThisFrame.empty()) {
    // per-tileset user-specified credit
    const Credit* pUserCredit = this->_pTilesetContentManager->getUserCredit();
    if (pUserCredit) {
      pCreditSystem->addCreditToFrame(*pUserCredit);
    }

    // tileset credit
    for (const Credit& credit : this->getTilesetCredits()) {
      pCreditSystem->addCreditToFrame(credit);
    }

    // per-raster overlay credit
    const RasterOverlayCollection& overlayCollection =
        this->_pTilesetContentManager->getRasterOverlayCollection();
    for (auto& pTileProvider : overlayCollection.getTileProviders()) {
      const std::optional<Credit>& overlayCredit = pTileProvider->getCredit();
      if (overlayCredit) {
        pCreditSystem->addCreditToFrame(overlayCredit.value());
      }
    }

    // per-tile credits
    for (Tile* pTile : result.tilesToRenderThisFrame) {
      const std::vector<RasterMappedTo3DTile>& mappedRasterTiles =
          pTile->getMappedRasterTiles();
      // raster overlay tile credits
      for (const RasterMappedTo3DTile& mappedRasterTile : mappedRasterTiles) {
        const RasterOverlayTile* pRasterOverlayTile =
            mappedRasterTile.getReadyTile();
        if (pRasterOverlayTile != nullptr) {
          for (const Credit& credit : pRasterOverlayTile->getCredits()) {
            pCreditSystem->addCreditToFrame(credit);
          }
        }
      }

      // content credits like gltf copyrights
      const TileRenderContent* pRenderContent =
          pTile->getContent().getRenderContent();
      if (pRenderContent) {
        for (const Credit& credit : pRenderContent->getCredits()) {
          pCreditSystem->addCreditToFrame(credit);
        }
      }
    }
  }

  this->_previousFrameNumber = currentFrameNumber;

  return result;
}
int32_t Tileset::getNumberOfTilesLoaded() const {
  return this->_pTilesetContentManager->getNumberOfTilesLoaded();
}

float Tileset::computeLoadProgress() noexcept {
  int32_t queueSizeSum = static_cast<int32_t>(
      this->_updateResult.workerThreadTileLoadQueueLength +
      this->_updateResult.mainThreadTileLoadQueueLength);
  int32_t numOfTilesLoading =
      this->_pTilesetContentManager->getNumberOfTilesLoading();
  int32_t numOfTilesLoaded =
      this->_pTilesetContentManager->getNumberOfTilesLoaded();
  int32_t numOfTilesKicked =
      static_cast<int32_t>(this->_updateResult.tilesKicked);

  // Amount of work actively being done
  int32_t inProgressSum = numOfTilesLoading + queueSizeSum;

  // Total work so far. Add already loaded tiles and kicked tiles.
  // Kicked tiles are transient, and never in progress, but are an indicator
  // that there is more work to do next frame.
  int32_t totalNum = inProgressSum + numOfTilesLoaded + numOfTilesKicked;
  float percentage =
      static_cast<float>(numOfTilesLoaded) / static_cast<float>(totalNum);
  return (percentage * 100.f);
}

void Tileset::forEachLoadedTile(
    const std::function<void(Tile& tile)>& callback) {
  Tile* pCurrent = this->_loadedTiles.head();
  while (pCurrent) {
    Tile* pNext = this->_loadedTiles.next(pCurrent);
    callback(*pCurrent);
    pCurrent = pNext;
  }
}

void Tileset::forEachLoadedTile(
    const std::function<void(const Tile& tile)>& callback) const {
  const Tile* pCurrent = this->_loadedTiles.head();
  while (pCurrent) {
    const Tile* pNext = this->_loadedTiles.next(pCurrent);
    callback(*pCurrent);
    pCurrent = pNext;
  }
}

int64_t Tileset::getTotalDataBytes() const noexcept {
  return this->_pTilesetContentManager->getTotalDataUsed();
}

const TilesetMetadata* Tileset::getMetadata(const Tile* pTile) const {
  if (pTile == nullptr) {
    pTile = this->getRootTile();
  }

  while (pTile != nullptr) {
    const TileExternalContent* pExternal =
        pTile->getContent().getExternalContent();
    if (pExternal)
      return &pExternal->metadata;
    pTile = pTile->getParent();
  }

  return nullptr;
}

CesiumAsync::Future<const TilesetMetadata*> Tileset::loadMetadata() {
  return this->getRootTileAvailableEvent().thenInMainThread(
      [pManager = this->_pTilesetContentManager,
       pAssetAccessor = this->_externals.pAssetAccessor,
       asyncSystem =
           this->getAsyncSystem()]() -> Future<const TilesetMetadata*> {
        Tile* pRoot = pManager->getRootTile();
        CESIUM_ASSERT(pRoot);

        TileExternalContent* pExternal =
            pRoot->getContent().getExternalContent();
        if (!pExternal) {
          return asyncSystem.createResolvedFuture<const TilesetMetadata*>(
              nullptr);
        }

        TilesetMetadata& metadata = pExternal->metadata;
        if (!metadata.schemaUri) {
          // No schema URI, so the metadata is ready to go.
          return asyncSystem.createResolvedFuture<const TilesetMetadata*>(
              &metadata);
        }

        return metadata.loadSchemaUri(asyncSystem, pAssetAccessor)
            .thenInMainThread(
                [pManager, pAssetAccessor]() -> const TilesetMetadata* {
                  Tile* pRoot = pManager->getRootTile();
                  CESIUM_ASSERT(pRoot);

                  TileExternalContent* pExternal =
                      pRoot->getContent().getExternalContent();
                  if (!pExternal) {
                    return nullptr;
                  }
                  return &pExternal->metadata;
                });
      });
}

CesiumAsync::Future<SampleHeightResult>
Tileset::sampleHeightMostDetailed(const std::vector<Cartographic>& positions) {
  if (positions.empty()) {
    return this->_asyncSystem.createResolvedFuture<SampleHeightResult>({});
  }

  Promise promise = this->_asyncSystem.createPromise<SampleHeightResult>();

  std::vector<TilesetHeightQuery> queries;
  queries.reserve(positions.size());

  for (const CesiumGeospatial::Cartographic& position : positions) {
    queries.emplace_back(position, this->_options.ellipsoid);
  }

  this->_heightRequests.emplace_back(
      TilesetHeightRequest{std::move(queries), promise});

  return promise.getFuture();
}

namespace {
void markTileNonRendered(
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Rendered ||
      (lastResult == TileSelectionState::Result::Refined &&
       tile.getRefine() == TileRefine::Add)) {
    result.tilesFadingOut.insert(&tile);
    TileRenderContent* pRenderContent = tile.getContent().getRenderContent();
    if (pRenderContent) {
      pRenderContent->setLodTransitionFadePercentage(0.0f);
    }
  }
}

void markTileNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markTileNonRendered(lastResult, tile, result);
}

void markChildrenNonRendered(
    int32_t lastFrameNumber,
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Refined) {
    for (Tile& child : tile.getChildren()) {
      const TileSelectionState::Result childLastResult =
          child.getLastSelectionState().getResult(lastFrameNumber);
      markTileNonRendered(childLastResult, child, result);
      markChildrenNonRendered(lastFrameNumber, childLastResult, child, result);
    }
  }
}

void markChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

void markTileAndChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markTileNonRendered(lastResult, tile, result);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

/**
 * @brief Returns whether a tile with the given bounding volume is visible for
 * the camera.
 *
 * @param viewState The {@link ViewState}
 * @param boundingVolume The bounding volume of the tile
 * @param forceRenderTilesUnderCamera Whether tiles under the camera should
 * always be considered visible and rendered (see
 * {@link Cesium3DTilesSelection::TilesetOptions}).
 * @return Whether the tile is visible according to the current camera
 * configuration
 */
bool isVisibleFromCamera(
    const ViewState& viewState,
    const BoundingVolume& boundingVolume,
    const Ellipsoid& ellipsoid,
    bool forceRenderTilesUnderCamera) {
  if (viewState.isBoundingVolumeVisible(boundingVolume)) {
    return true;
  }
  if (!forceRenderTilesUnderCamera) {
    return false;
  }

  const std::optional<CesiumGeospatial::Cartographic>& position =
      viewState.getPositionCartographic();

  // TODO: it would be better to test a line pointing down (and up?) from the
  // camera against the bounding volume itself, rather than transforming the
  // bounding volume to a region.
  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume, ellipsoid);
  if (position && maybeRectangle) {
    return maybeRectangle->contains(position.value());
  }
  return false;
}

/**
 * @brief Returns whether a tile at the given distance is visible in the fog.
 *
 * @param distance The distance of the tile bounding volume to the camera
 * @param fogDensity The fog density
 * @return Whether the tile is visible in the fog
 */
bool isVisibleInFog(double distance, double fogDensity) noexcept {
  if (fogDensity <= 0.0) {
    return true;
  }

  const double fogScalar = distance * fogDensity;
  return glm::exp(-(fogScalar * fogScalar)) > 0.0;
}
} // namespace

void Tileset::_frustumCull(
    const Tile& tile,
    const FrameState& frameState,
    bool cullWithChildrenBounds,
    CullResult& cullResult) {

  if (!cullResult.shouldVisit || cullResult.culled) {
    return;
  }

  const CesiumGeospatial::Ellipsoid& ellipsoid = this->getEllipsoid();

  const std::vector<ViewState>& frustums = frameState.frustums;
  // Frustum cull using the children's bounds.
  if (cullWithChildrenBounds) {
    if (std::any_of(
            frustums.begin(),
            frustums.end(),
            [&ellipsoid,
             children = tile.getChildren(),
             renderTilesUnderCamera = this->_options.renderTilesUnderCamera](
                const ViewState& frustum) {
              for (const Tile& child : children) {
                if (isVisibleFromCamera(
                        frustum,
                        child.getBoundingVolume(),
                        ellipsoid,
                        renderTilesUnderCamera)) {
                  return true;
                }
              }

              return false;
            })) {
      // At least one child is visible in at least one frustum, so don't cull.
      return;
    }
    // Frustum cull based on the actual tile's bounds.
  } else if (std::any_of(
                 frustums.begin(),
                 frustums.end(),
                 [&ellipsoid,
                  &boundingVolume = tile.getBoundingVolume(),
                  renderTilesUnderCamera =
                      this->_options.renderTilesUnderCamera](
                     const ViewState& frustum) {
                   return isVisibleFromCamera(
                       frustum,
                       boundingVolume,
                       ellipsoid,
                       renderTilesUnderCamera);
                 })) {
    // The tile is visible in at least one frustum, so don't cull.
    return;
  }

  // If we haven't returned yet, this tile is frustum culled.
  cullResult.culled = true;

  if (this->_options.enableFrustumCulling) {
    // frustum culling is enabled so we shouldn't visit this off-screen tile
    cullResult.shouldVisit = false;
  }
}

void Tileset::_fogCull(
    const FrameState& frameState,
    const std::vector<double>& distances,
    CullResult& cullResult) {

  if (!cullResult.shouldVisit || cullResult.culled) {
    return;
  }

  const std::vector<ViewState>& frustums = frameState.frustums;
  const std::vector<double>& fogDensities = frameState.fogDensities;

  bool isFogCulled = true;

  for (size_t i = 0; i < frustums.size(); ++i) {
    const double distance = distances[i];
    const double fogDensity = fogDensities[i];

    if (isVisibleInFog(distance, fogDensity)) {
      isFogCulled = false;
      break;
    }
  }

  if (isFogCulled) {
    // this tile is occluded by fog so it is a culled tile
    cullResult.culled = true;
    if (this->_options.enableFogCulling) {
      // fog culling is enabled so we shouldn't visit this tile
      cullResult.shouldVisit = false;
    }
  }
}

namespace {

double computeTilePriority(
    const Tile& tile,
    const std::vector<ViewState>& frustums,
    const std::vector<double>& distances) {
  double highestLoadPriority = std::numeric_limits<double>::max();
  const glm::dvec3 boundingVolumeCenter =
      getBoundingVolumeCenter(tile.getBoundingVolume());

  for (size_t i = 0; i < frustums.size() && i < distances.size(); ++i) {
    const ViewState& frustum = frustums[i];
    const double distance = distances[i];

    glm::dvec3 tileDirection = boundingVolumeCenter - frustum.getPosition();
    const double magnitude = glm::length(tileDirection);

    if (magnitude >= CesiumUtility::Math::Epsilon5) {
      tileDirection /= magnitude;
      const double loadPriority =
          (1.0 - glm::dot(tileDirection, frustum.getDirection())) * distance;
      if (loadPriority < highestLoadPriority) {
        highestLoadPriority = loadPriority;
      }
    }
  }

  return highestLoadPriority;
}

void computeDistances(
    const Tile& tile,
    const std::vector<ViewState>& frustums,
    std::vector<double>& distances) {
  const BoundingVolume& boundingVolume = tile.getBoundingVolume();

  distances.clear();
  distances.resize(frustums.size());

  std::transform(
      frustums.begin(),
      frustums.end(),
      distances.begin(),
      [boundingVolume](const ViewState& frustum) -> double {
        return glm::sqrt(glm::max(
            frustum.computeDistanceSquaredToBoundingVolume(boundingVolume),
            0.0));
      });
}

} // namespace

bool Tileset::_meetsSse(
    const std::vector<ViewState>& frustums,
    const Tile& tile,
    const std::vector<double>& distances,
    bool culled) const noexcept {

  double largestSse = 0.0;

  for (size_t i = 0; i < frustums.size() && i < distances.size(); ++i) {
    const ViewState& frustum = frustums[i];
    const double distance = distances[i];

    // Does this tile meet the screen-space error?
    const double sse =
        frustum.computeScreenSpaceError(tile.getGeometricError(), distance);
    if (sse > largestSse) {
      largestSse = sse;
    }
  }

  return culled ? !this->_options.enforceCulledScreenSpaceError ||
                      largestSse < this->_options.culledScreenSpaceError
                : largestSse < this->_options.maximumScreenSpaceError;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * It is not yet known whether the tile is visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTileIfNeeded(
    const FrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {

  std::vector<double>& distances = this->_distances;
  computeDistances(tile, frameState.frustums, distances);
  double tilePriority =
      computeTilePriority(tile, frameState.frustums, distances);

  this->_pTilesetContentManager->updateTileContent(tile, _options);
  this->_markTileVisited(tile);

  CullResult cullResult{};

  // Culling with children bounds will give us incorrect results with Add
  // refinement, but is a useful optimization for Replace refinement.
  bool cullWithChildrenBounds =
      tile.getRefine() == TileRefine::Replace && !tile.getChildren().empty();
  for (Tile& child : tile.getChildren()) {
    if (child.getUnconditionallyRefine()) {
      cullWithChildrenBounds = false;
      break;
    }
  }

  // TODO: add cullWithChildrenBounds to the tile excluder interface?
  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       this->_options.excluders) {
    if (pExcluder->shouldExclude(tile)) {
      cullResult.culled = true;
      cullResult.shouldVisit = false;
      break;
    }
  }

  // TODO: abstract culling stages into composable interface?
  this->_frustumCull(tile, frameState, cullWithChildrenBounds, cullResult);
  this->_fogCull(frameState, distances, cullResult);

  if (!cullResult.shouldVisit && tile.getUnconditionallyRefine()) {
    // Unconditionally refined tiles must always be visited in forbidHoles
    // mode, because we need to load this tile's descendants before we can
    // render any of its siblings. An unconditionally refined root tile must be
    // visited as well, otherwise we won't load anything at all.
    if ((this->_options.forbidHoles &&
         tile.getRefine() == TileRefine::Replace) ||
        tile.getParent() == nullptr) {
      cullResult.shouldVisit = true;
    }
  }

  if (!cullResult.shouldVisit) {
    const TileSelectionState lastFrameSelectionState =
        tile.getLastSelectionState();

    markTileAndChildrenNonRendered(frameState.lastFrameNumber, tile, result);
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Culled));

    ++result.tilesCulled;

    TraversalDetails traversalDetails{};

    if (this->_options.forbidHoles && tile.getRefine() == TileRefine::Replace) {
      // In order to prevent holes, we need to load this tile and also not
      // render any siblings until it is ready. We don't actually need to
      // render it, though.
      addTileToLoadQueue(tile, TileLoadPriorityGroup::Normal, tilePriority);

      traversalDetails = Tileset::createTraversalDetailsForSingleTile(
          frameState,
          tile,
          lastFrameSelectionState);
    } else if (this->_options.preloadSiblings) {
      // Preload this culled sibling as requested.
      addTileToLoadQueue(tile, TileLoadPriorityGroup::Preload, tilePriority);
    }

    return traversalDetails;
  }

  if (cullResult.culled) {
    ++result.culledTilesVisited;
  }

  bool meetsSse =
      this->_meetsSse(frameState.frustums, tile, distances, cullResult.culled);

  return this->_visitTile(
      frameState,
      depth,
      meetsSse,
      ancestorMeetsSse,
      tile,
      tilePriority,
      result);
}

namespace {
bool isLeaf(const Tile& tile) noexcept { return tile.getChildren().empty(); }
} // namespace

Tileset::TraversalDetails Tileset::_renderLeaf(
    const FrameState& frameState,
    Tile& tile,
    double tilePriority,
    ViewUpdateResult& result) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);

  addTileToLoadQueue(tile, TileLoadPriorityGroup::Normal, tilePriority);

  return Tileset::createTraversalDetailsForSingleTile(
      frameState,
      tile,
      lastFrameSelectionState);
}

namespace {

/**
 * @brief Determines if we must refine this tile so that we can continue
 * rendering the deeper descendant tiles of this tile.
 *
 * If this tile was refined last frame, and is not yet renderable, then we
 * should REFINE past this tile in order to continue rendering the deeper tiles
 * that we rendered last frame, until such time as this tile is loaded and we
 * can render it instead. This is necessary to avoid detail vanishing when
 * the camera zooms out and lower-detail tiles are not yet loaded.
 *
 * @param tile The tile to check, which is assumed to meet the SSE for
 * rendering.
 * @param lastFrameSelectionState The selection state of this tile last frame.
 * @param lastFrameNumber The previous frame number.
 * @return True if this tile must be refined instead of rendered, so that we can
 * continue rendering deeper tiles.
 */
bool mustContinueRefiningToDeeperTiles(
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState,
    int32_t lastFrameNumber) noexcept {
  const TileSelectionState::Result originalResult =
      lastFrameSelectionState.getOriginalResult(lastFrameNumber);

  return originalResult == TileSelectionState::Result::Refined &&
         !tile.isRenderable();
}

} // namespace

Tileset::TraversalDetails Tileset::_renderInnerTile(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  markChildrenNonRendered(frameState.lastFrameNumber, tile, result);
  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);

  return Tileset::createTraversalDetailsForSingleTile(
      frameState,
      tile,
      lastFrameSelectionState);
}

bool Tileset::_loadAndRenderAdditiveRefinedTile(
    Tile& tile,
    ViewUpdateResult& result,
    double tilePriority,
    bool queuedForLoad) {
  // If this tile uses additive refinement, we need to render this tile in
  // addition to its children.
  if (tile.getRefine() == TileRefine::Add) {
    result.tilesToRenderThisFrame.push_back(&tile);
    if (!queuedForLoad)
      addTileToLoadQueue(tile, TileLoadPriorityGroup::Normal, tilePriority);
    return true;
  }

  return false;
}

// TODO This function is obviously too complex. The way how the indices are
// used, in order to deal with the queue elements, should be reviewed...
bool Tileset::_kickDescendantsAndRenderTile(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    TraversalDetails& traversalDetails,
    size_t firstRenderedDescendantIndex,
    size_t workerThreadLoadQueueIndex,
    size_t mainThreadLoadQueueIndex,
    bool queuedForLoad,
    double tilePriority) {
  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  std::vector<Tile*>& renderList = result.tilesToRenderThisFrame;

  // Mark the rendered descendants and their ancestors - up to this tile - as
  // kicked.
  for (size_t i = firstRenderedDescendantIndex; i < renderList.size(); ++i) {
    Tile* pWorkTile = renderList[i];
    while (pWorkTile != nullptr &&
           !pWorkTile->getLastSelectionState().wasKicked(
               frameState.currentFrameNumber) &&
           pWorkTile != &tile) {
      pWorkTile->getLastSelectionState().kick();
      pWorkTile = pWorkTile->getParent();
    }
  }

  // Remove all descendants from the render list and add this tile.
  renderList.erase(
      renderList.begin() +
          static_cast<std::vector<Tile*>::iterator::difference_type>(
              firstRenderedDescendantIndex),
      renderList.end());

  if (tile.getRefine() != Cesium3DTilesSelection::TileRefine::Add) {
    renderList.push_back(&tile);
  }

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));

  // If we're waiting on heaps of descendants, the above will take too long. So
  // in that case, load this tile INSTEAD of loading any of the descendants, and
  // tell the up-level we're only waiting on this tile. Keep doing this until we
  // actually manage to render this tile.
  // Make sure we don't end up waiting on a tile that will _never_ be
  // renderable.
  const bool wasRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  const bool wasReallyRenderedLastFrame =
      wasRenderedLastFrame && tile.isRenderable();

  if (!wasReallyRenderedLastFrame &&
      traversalDetails.notYetRenderableCount >
          this->_options.loadingDescendantLimit &&
      !tile.isExternalContent() && !tile.getUnconditionallyRefine()) {

    // Remove all descendants from the load queues.
    size_t allQueueStartSize =
        _workerThreadLoadQueue.size() + _mainThreadLoadQueue.size();
    this->_workerThreadLoadQueue.erase(
        this->_workerThreadLoadQueue.begin() +
            static_cast<std::vector<TileLoadTask>::iterator::difference_type>(
                workerThreadLoadQueueIndex),
        this->_workerThreadLoadQueue.end());
    this->_mainThreadLoadQueue.erase(
        this->_mainThreadLoadQueue.begin() +
            static_cast<std::vector<TileLoadTask>::iterator::difference_type>(
                mainThreadLoadQueueIndex),
        this->_mainThreadLoadQueue.end());
    size_t allQueueEndSize =
        _workerThreadLoadQueue.size() + _mainThreadLoadQueue.size();
    result.tilesKicked +=
        static_cast<uint32_t>(allQueueStartSize - allQueueEndSize);

    if (!queuedForLoad) {
      addTileToLoadQueue(tile, TileLoadPriorityGroup::Normal, tilePriority);
    }

    traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
    queuedForLoad = true;
  }

  bool isRenderable = tile.isRenderable();
  traversalDetails.allAreRenderable = isRenderable;
  traversalDetails.anyWereRenderedLastFrame =
      isRenderable && wasRenderedLastFrame;

  return queuedForLoad;
}

TileOcclusionState
Tileset::_checkOcclusion(const Tile& tile, const FrameState& frameState) {
  const std::shared_ptr<TileOcclusionRendererProxyPool>& pOcclusionPool =
      this->_externals.pTileOcclusionProxyPool;
  if (pOcclusionPool) {
    // First check if this tile's bounding volume has occlusion info and is
    // known to be occluded.
    const TileOcclusionRendererProxy* pOcclusion =
        pOcclusionPool->fetchOcclusionProxyForTile(
            tile,
            frameState.currentFrameNumber);
    if (!pOcclusion) {
      // This indicates we ran out of occlusion proxies. We don't want to wait
      // on occlusion info here since it might not ever arrive, so treat this
      // tile as if it is _known_ to be unoccluded.
      return TileOcclusionState::NotOccluded;
    } else
      switch (
          static_cast<TileOcclusionState>(pOcclusion->getOcclusionState())) {
      case TileOcclusionState::OcclusionUnavailable:
        // We have an occlusion proxy, but it does not have valid occlusion
        // info yet, wait for it.
        return TileOcclusionState::OcclusionUnavailable;
        break;
      case TileOcclusionState::Occluded:
        return TileOcclusionState::Occluded;
        break;
      case TileOcclusionState::NotOccluded:
        if (tile.getChildren().size() == 0) {
          // This is a leaf tile, so we can't use children bounding volumes.
          return TileOcclusionState::NotOccluded;
        }
      }

    // The tile's bounding volume is known to be unoccluded, but check the
    // union of the children bounding volumes since it is tighter fitting.

    // If any children are to be unconditionally refined, we can't rely on
    // their bounding volumes. We also don't want to recurse indefinitely to
    // find a valid descendant bounding volumes union.
    for (const Tile& child : tile.getChildren()) {
      if (child.getUnconditionallyRefine()) {
        return TileOcclusionState::NotOccluded;
      }
    }

    this->_childOcclusionProxies.clear();
    this->_childOcclusionProxies.reserve(tile.getChildren().size());
    for (const Tile& child : tile.getChildren()) {
      const TileOcclusionRendererProxy* pChildProxy =
          pOcclusionPool->fetchOcclusionProxyForTile(
              child,
              frameState.currentFrameNumber);

      if (!pChildProxy) {
        // We ran out of occlusion proxies, treat this as if it is _known_ to
        // be unoccluded so we don't wait for it.
        return TileOcclusionState::NotOccluded;
      }

      this->_childOcclusionProxies.push_back(pChildProxy);
    }

    // Check if any of the proxies are known to be unoccluded
    for (const TileOcclusionRendererProxy* pChildProxy :
         this->_childOcclusionProxies) {
      if (pChildProxy->getOcclusionState() == TileOcclusionState::NotOccluded) {
        return TileOcclusionState::NotOccluded;
      }
    }

    // Check if any of the proxies are waiting for valid occlusion info.
    for (const TileOcclusionRendererProxy* pChildProxy :
         this->_childOcclusionProxies) {
      if (pChildProxy->getOcclusionState() ==
          TileOcclusionState::OcclusionUnavailable) {
        // We have an occlusion proxy, but it does not have valid occlusion
        // info yet, wait for it.
        return TileOcclusionState::OcclusionUnavailable;
      }
    }

    // If we know the occlusion state of all children, and none are unoccluded,
    // we can treat this tile as occluded.
    return TileOcclusionState::Occluded;
  }

  // We don't have an occlusion pool to query occlusion with, treat everything
  // as unoccluded.
  return TileOcclusionState::NotOccluded;
}

namespace {

enum class VisitTileAction { Render, Refine };

}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * The tile has previously been determined to be visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTile(
    const FrameState& frameState,
    uint32_t depth,
    bool meetsSse,
    bool ancestorMeetsSse, // Careful: May be modified before being passed to
                           // children!
    Tile& tile,
    double tilePriority,
    ViewUpdateResult& result) {
  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  // If this is a leaf tile, just render it (it's already been deemed visible).
  if (isLeaf(tile)) {
    return _renderLeaf(frameState, tile, tilePriority, result);
  }

  const bool unconditionallyRefine = tile.getUnconditionallyRefine();
  const bool refineForSse = !meetsSse && !ancestorMeetsSse;

  // Determine whether to REFINE or RENDER. Note that even if this tile is
  // initially marked for RENDER here, it may later switch to REFINE as a
  // result of `mustContinueRefiningToDeeperTiles`.
  VisitTileAction action;
  if (unconditionallyRefine || refineForSse)
    action = VisitTileAction::Refine;
  else
    action = VisitTileAction::Render;

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();
  const TileSelectionState::Result lastFrameSelectionResult =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber);

  // If occlusion culling is enabled, we may not want to refine for two
  // reasons:
  // - The tile is known to be occluded, so don't refine further.
  // - The tile was not previously refined and the occlusion state for this
  //   tile is not known yet, but will be known in the next several frames. If
  //   delayRefinementForOcclusion is enabled, we will wait until the tile has
  //   valid occlusion info to decide to refine. This might save us from
  //   kicking off descendant loads that we later find to be unnecessary.
  bool tileLastRefined =
      lastFrameSelectionResult == TileSelectionState::Result::Refined;
  bool childLastRefined = false;
  for (const Tile& child : tile.getChildren()) {
    if (child.getLastSelectionState().getResult(frameState.lastFrameNumber) ==
        TileSelectionState::Result::Refined) {
      childLastRefined = true;
      break;
    }
  }

  // If this tile and a child were both refined last frame, this tile does not
  // need occlusion results.
  bool shouldCheckOcclusion = this->_options.enableOcclusionCulling &&
                              action == VisitTileAction::Refine &&
                              !unconditionallyRefine &&
                              (!tileLastRefined || !childLastRefined);

  if (shouldCheckOcclusion) {
    TileOcclusionState occlusion = this->_checkOcclusion(tile, frameState);
    if (occlusion == TileOcclusionState::Occluded) {
      ++result.tilesOccluded;
      action = VisitTileAction::Render;
      meetsSse = true;
    } else if (
        occlusion == TileOcclusionState::OcclusionUnavailable &&
        this->_options.delayRefinementForOcclusion &&
        tile.getLastSelectionState().getOriginalResult(
            frameState.lastFrameNumber) !=
            TileSelectionState::Result::Refined) {
      ++result.tilesWaitingForOcclusionResults;
      action = VisitTileAction::Render;
      meetsSse = true;
    }
  }

  bool queuedForLoad = false;

  if (action == VisitTileAction::Render) {
    // This tile meets the screen-space error requirement, so we'd like to
    // render it, if we can.
    bool mustRefine = mustContinueRefiningToDeeperTiles(
        tile,
        lastFrameSelectionState,
        frameState.lastFrameNumber);
    if (mustRefine) {
      // // We must refine even though this tile meets the SSE.
      action = VisitTileAction::Refine;

      // Loading this tile is very important, because a number of deeper,
      // higher-detail tiles are being rendered in its stead, so we want to load
      // it with high priority. However, if `ancestorMeetsSse` is set, then our
      // parent tile is in the exact same situation, and loading this tile with
      // high priority would compete with that one. We should prefer the parent
      // because it is closest to the actual desired LOD and because up the tree
      // there can only be fewer tiles that need loading.
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(tile, TileLoadPriorityGroup::Urgent, tilePriority);
        queuedForLoad = true;
      }

      // Fall through to REFINE, but mark this tile as already meeting the
      // required SSE.
      ancestorMeetsSse = true;
    } else {
      // Render this tile and return without visiting children.
      // Only load this tile if it (not just an ancestor) meets the SSE.
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(tile, TileLoadPriorityGroup::Normal, tilePriority);
      }
      return _renderInnerTile(frameState, tile, result);
    }
  }

  // Refine!

  queuedForLoad = _loadAndRenderAdditiveRefinedTile(
                      tile,
                      result,
                      tilePriority,
                      queuedForLoad) ||
                  queuedForLoad;

  const size_t firstRenderedDescendantIndex =
      result.tilesToRenderThisFrame.size();
  const size_t workerThreadLoadQueueIndex = this->_workerThreadLoadQueue.size();
  const size_t mainThreadLoadQueueIndex = this->_mainThreadLoadQueue.size();

  TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      result);

  // Zero or more descendant tiles were added to the render list.
  // The traversalDetails tell us what happened while visiting the children.

  // Descendants will be kicked if any are not ready to render yet and none
  // were rendered last frame.
  bool kickDueToNonReadyDescendant = !traversalDetails.allAreRenderable &&
                                     !traversalDetails.anyWereRenderedLastFrame;

  // Descendants may also be kicked if this tile was rendered last frame and
  // has not finished fading in yet.
  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();
  bool kickDueToTileFadingIn =
      _options.enableLodTransitionPeriod &&
      _options.kickDescendantsWhileFadingIn &&
      lastFrameSelectionResult == TileSelectionState::Result::Rendered &&
      pRenderContent && pRenderContent->getLodTransitionFadePercentage() < 1.0f;

  // Only kick the descendants of this tile if it is renderable, or if we've
  // exceeded the loadingDescendantLimit. It's pointless to kick the descendants
  // of a tile that is not yet loaded, because it means we will still have a
  // hole, and quite possibly a bigger one.
  bool wantToKick = kickDueToNonReadyDescendant || kickDueToTileFadingIn;
  bool willKick = wantToKick && (traversalDetails.notYetRenderableCount >
                                     this->_options.loadingDescendantLimit ||
                                 tile.isRenderable());

  if (willKick) {
    // Kick all descendants out of the render list and render this tile instead
    // Continue to load them though!
    queuedForLoad = _kickDescendantsAndRenderTile(
        frameState,
        tile,
        result,
        traversalDetails,
        firstRenderedDescendantIndex,
        workerThreadLoadQueueIndex,
        mainThreadLoadQueueIndex,
        queuedForLoad,
        tilePriority);
  } else {
    if (tile.getRefine() != TileRefine::Add) {
      markTileNonRendered(frameState.lastFrameNumber, tile, result);
    }
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Refined));
  }

  if (this->_options.preloadAncestors && !queuedForLoad) {
    addTileToLoadQueue(tile, TileLoadPriorityGroup::Preload, tilePriority);
  }

  return traversalDetails;
}

Tileset::TraversalDetails Tileset::_visitVisibleChildrenNearToFar(
    const FrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {
  TraversalDetails traversalDetails;

  // TODO: actually visit near-to-far, rather than in order of occurrence.
  std::span<Tile> children = tile.getChildren();
  for (Tile& child : children) {
    const TraversalDetails childTraversal = this->_visitTileIfNeeded(
        frameState,
        depth + 1,
        ancestorMeetsSse,
        child,
        result);

    traversalDetails.allAreRenderable &= childTraversal.allAreRenderable;
    traversalDetails.anyWereRenderedLastFrame |=
        childTraversal.anyWereRenderedLastFrame;
    traversalDetails.notYetRenderableCount +=
        childTraversal.notYetRenderableCount;
  }

  return traversalDetails;
}

void Tileset::_processWorkerThreadLoadQueue() {
  CESIUM_TRACE("Tileset::_processWorkerThreadLoadQueue");

  int32_t maximumSimultaneousTileLoads =
      static_cast<int32_t>(this->_options.maximumSimultaneousTileLoads);

  if (this->_pTilesetContentManager->getNumberOfTilesLoading() >=
      maximumSimultaneousTileLoads) {
    return;
  }

  std::sort(
      this->_workerThreadLoadQueue.begin(),
      this->_workerThreadLoadQueue.end());

  // Select tiles alternately from the two queues. Each frame, switch which
  // queue we pull the first tile from. The goal is to schedule both height
  // query and visualization tile loads fairly.
  auto visIt = this->_workerThreadLoadQueue.begin();
  auto queryIt = this->_heightQueryLoadQueue.begin();

  bool nextIsVis = (this->_previousFrameNumber % 2) == 0;

  while (this->_pTilesetContentManager->getNumberOfTilesLoading() <
         maximumSimultaneousTileLoads) {
    // Tell tiles from the current queue to load until one of them actually
    // does. Calling loadTileContent might not actually start the loading
    // process
    int32_t originalNumberOfTilesLoading =
        this->_pTilesetContentManager->getNumberOfTilesLoading();
    if (nextIsVis) {
      while (visIt != this->_workerThreadLoadQueue.end() &&
             originalNumberOfTilesLoading ==
                 this->_pTilesetContentManager->getNumberOfTilesLoading()) {
        this->_pTilesetContentManager->loadTileContent(*visIt->pTile, _options);
        ++visIt;
      }
    } else {
      while (queryIt != this->_heightQueryLoadQueue.end() &&
             originalNumberOfTilesLoading ==
                 this->_pTilesetContentManager->getNumberOfTilesLoading()) {
        this->_pTilesetContentManager->loadTileContent(**queryIt, _options);
        ++queryIt;
      }
    }

    if (visIt == this->_workerThreadLoadQueue.end() &&
        queryIt == this->_heightQueryLoadQueue.end()) {
      // No more work in either queue
      break;
    }

    // Get the next tile from the other queue.
    nextIsVis = !nextIsVis;
  }
}

void Tileset::_processMainThreadLoadQueue() {
  CESIUM_TRACE("Tileset::_processMainThreadLoadQueue");
  // Process deferred main-thread load tasks with a time budget.

  std::sort(
      this->_mainThreadLoadQueue.begin(),
      this->_mainThreadLoadQueue.end());

  double timeBudget = this->_options.mainThreadLoadingTimeLimit;

  auto start = std::chrono::system_clock::now();
  auto end = start + std::chrono::microseconds(
                         static_cast<int64_t>(1000.0 * timeBudget));
  for (TileLoadTask& task : this->_mainThreadLoadQueue) {
    // We double-check that the tile is still in the ContentLoaded state here,
    // in case something (such as a child that needs to upsample from this
    // parent) already pushed the tile into the Done state. Because in that
    // case, calling finishLoading here would assert or crash.
    if (task.pTile->getState() == TileLoadState::ContentLoaded &&
        task.pTile->isRenderContent()) {
      this->_pTilesetContentManager->finishLoading(*task.pTile, this->_options);
    }
    auto time = std::chrono::system_clock::now();
    if (timeBudget > 0.0 && time >= end) {
      break;
    }
  }

  this->_mainThreadLoadQueue.clear();
}

void Tileset::_unloadCachedTiles(double timeBudget) noexcept {
  const int64_t maxBytes = this->getOptions().maximumCachedBytes;

  const Tile* pRootTile = this->_pTilesetContentManager->getRootTile();
  Tile* pTile = this->_loadedTiles.head();

  // A time budget of 0.0 indicates we shouldn't throttle cache unloads. So set
  // the end time to the max time_point in that case.
  auto start = std::chrono::system_clock::now();
  auto end = (timeBudget <= 0.0)
                 ? std::chrono::time_point<std::chrono::system_clock>::max()
                 : (start + std::chrono::microseconds(
                                static_cast<int64_t>(1000.0 * timeBudget)));

  while (this->getTotalDataBytes() > maxBytes) {
    if (pTile == nullptr || pTile == pRootTile) {
      // We've either removed all tiles or the next tile is the root.
      // The root tile marks the beginning of the tiles that were used
      // for rendering last frame.
      break;
    }

    // Don't unload this tile if it is still fading out.
    if (_updateResult.tilesFadingOut.find(pTile) !=
        _updateResult.tilesFadingOut.end()) {
      pTile = this->_loadedTiles.next(*pTile);
      continue;
    }

    Tile* pNext = this->_loadedTiles.next(*pTile);

    const bool removed =
        this->_pTilesetContentManager->unloadTileContent(*pTile);
    if (removed) {
      this->_loadedTiles.remove(*pTile);
    }

    pTile = pNext;

    auto time = std::chrono::system_clock::now();
    if (time >= end) {
      break;
    }
  }
}

void Tileset::_markTileVisited(Tile& tile) noexcept {
  this->_loadedTiles.insertAtTail(tile);
}

void Tileset::addTileToLoadQueue(
    Tile& tile,
    TileLoadPriorityGroup priorityGroup,
    double priority) {
  // Assert that this tile hasn't been added to a queue already.
  CESIUM_ASSERT(
      std::find_if(
          this->_workerThreadLoadQueue.begin(),
          this->_workerThreadLoadQueue.end(),
          [&](const TileLoadTask& task) { return task.pTile == &tile; }) ==
      this->_workerThreadLoadQueue.end());
  CESIUM_ASSERT(
      std::find_if(
          this->_mainThreadLoadQueue.begin(),
          this->_mainThreadLoadQueue.end(),
          [&](const TileLoadTask& task) { return task.pTile == &tile; }) ==
      this->_mainThreadLoadQueue.end());

  if (this->_pTilesetContentManager->tileNeedsWorkerThreadLoading(tile)) {
    this->_workerThreadLoadQueue.push_back({&tile, priorityGroup, priority});
  } else if (this->_pTilesetContentManager->tileNeedsMainThreadLoading(tile)) {
    this->_mainThreadLoadQueue.push_back({&tile, priorityGroup, priority});
  }
}

Tileset::TraversalDetails Tileset::createTraversalDetailsForSingleTile(
    const FrameState& frameState,
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState) {
  TileSelectionState::Result lastFrameResult =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber);
  bool isRenderable = tile.isRenderable();

  bool wasRenderedLastFrame =
      lastFrameResult == TileSelectionState::Result::Rendered;
  if (!wasRenderedLastFrame &&
      lastFrameResult == TileSelectionState::Result::Refined) {
    if (tile.getRefine() == TileRefine::Add) {
      // An additive-refined tile that was refined was also rendered.
      wasRenderedLastFrame = true;
    } else {
      // With replace-refinement, if any of this refined tile's children were
      // rendered last frame, but are no longer rendered because this tile is
      // loaded and has sufficient detail, we must treat this tile as rendered
      // last frame, too. This is necessary to prevent this tile from being
      // kicked just because _it_ wasn't rendered last frame (which could cause
      // a new hole to appear).
      for (const Tile& child : tile.getChildren()) {
        TraversalDetails childDetails = createTraversalDetailsForSingleTile(
            frameState,
            child,
            child.getLastSelectionState());
        wasRenderedLastFrame |= childDetails.anyWereRenderedLastFrame;
      }
    }
  }

  TraversalDetails traversalDetails;
  traversalDetails.allAreRenderable = isRenderable;
  traversalDetails.anyWereRenderedLastFrame =
      isRenderable && wasRenderedLastFrame;
  traversalDetails.notYetRenderableCount = isRenderable ? 0 : 1;

  return traversalDetails;
}

} // namespace Cesium3DTilesSelection
