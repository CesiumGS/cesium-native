#include "Cesium3DTilesSelection/Tileset.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/ExternalTilesetContent.h"
#include "Cesium3DTilesSelection/ITileExcluder.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "QuantizedMeshContent.h"
#include "TileUtilities.h"
#include "TilesetLoadIonAssetEndpoint.h"
#include "TilesetLoadSubtree.h"
#include "TilesetLoadTileFromJson.h"
#include "TilesetLoadTilesetDotJson.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/TileAvailabilityFlags.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/ScopeGuard.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/Uri.h>

#include <glm/common.hpp>
#include <rapidjson/document.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <unordered_set>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

Tileset::Tileset(
    const TilesetExternals& externals,
    const std::string& url,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value(),
                    options.showCreditsOnScreen))
              : std::nullopt),
      _url(url),
      _isRefreshingIonToken(false),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _loadsInProgress(0),
      _subtreeLoadsInProgress(0),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false),
      _gltfUpAxis(CesiumGeometry::Axis::Y),
      _distancesStack(),
      _nextDistancesVector(0) {
  if (!url.empty()) {
    CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
    this->notifyTileStartLoading(nullptr);
    LoadTilesetDotJson::start(*this, url).thenInMainThread([this]() {
      this->notifyTileDoneLoading(nullptr);
    });
  }
}

Tileset::Tileset(
    const TilesetExternals& externals,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const TilesetOptions& options,
    const std::string& ionAssetEndpointUrl)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value(),
                    options.showCreditsOnScreen))
              : std::nullopt),
      _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _isRefreshingIonToken(false),
      _ionAssetEndpointUrl(ionAssetEndpointUrl),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _loadsInProgress(0),
      _subtreeLoadsInProgress(0),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false),
      _gltfUpAxis(CesiumGeometry::Axis::Y),
      _distancesStack(),
      _nextDistancesVector(0) {
  if (ionAssetID > 0) {
    CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
    this->notifyTileStartLoading(nullptr);
    LoadIonAssetEndpoint::start(*this).thenInMainThread(
        [this]() { this->notifyTileDoneLoading(nullptr); });
  }
}

Tileset::~Tileset() {
  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _loadsInProgress not
  // being decremented correctly when an async load ends.
  while (this->_loadsInProgress.load(std::memory_order::memory_order_acquire) >
             0 ||
         this->_subtreeLoadsInProgress.load(
             std::memory_order::memory_order_acquire) > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_asyncSystem.dispatchMainThreadTasks();
  }

  // Wait for all overlays to wrap up their loading, too.
  uint32_t tilesLoading = 1;
  while (tilesLoading > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_asyncSystem.dispatchMainThreadTasks();

    tilesLoading = 0;
    for (auto& pOverlay : this->_overlays) {
      tilesLoading += pOverlay->getTileProvider()->getNumberOfTilesLoading();
    }
  }
}

static bool
operator<(const FogDensityAtHeight& fogDensity, double height) noexcept {
  return fogDensity.cameraHeight < height;
}

static double computeFogDensity(
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

const ViewUpdateResult&
Tileset::updateViewOffline(const std::vector<ViewState>& frustums) {
  std::vector<Tile*> tilesRenderedPrevFrame =
      this->_updateResult.tilesToRenderThisFrame;

  this->updateView(frustums);
  while (this->_loadsInProgress > 0 || this->_subtreeLoadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->updateView(frustums);
  }

  std::unordered_set<Tile*> uniqueTilesToRenderedThisFrame(
      this->_updateResult.tilesToRenderThisFrame.begin(),
      this->_updateResult.tilesToRenderThisFrame.end());
  std::vector<Tile*> tilesToNoLongerRenderThisFrame;
  for (Tile* tile : tilesRenderedPrevFrame) {
    if (uniqueTilesToRenderedThisFrame.find(tile) ==
        uniqueTilesToRenderedThisFrame.end()) {
      tilesToNoLongerRenderThisFrame.emplace_back(tile);
    }
  }

  this->_updateResult.tilesToNoLongerRenderThisFrame =
      std::move(tilesToNoLongerRenderThisFrame);
  return this->_updateResult;
}

const ViewUpdateResult&
Tileset::updateView(const std::vector<ViewState>& frustums) {
  this->_asyncSystem.dispatchMainThreadTasks();

  const int32_t previousFrameNumber = this->_previousFrameNumber;
  const int32_t currentFrameNumber = previousFrameNumber + 1;

  ViewUpdateResult& result = this->_updateResult;
  // result.tilesLoading = 0;
  result.tilesToRenderThisFrame.clear();
  // result.newTilesToRenderThisFrame.clear();
  result.tilesToNoLongerRenderThisFrame.clear();
  result.tilesVisited = 0;
  result.culledTilesVisited = 0;
  result.tilesCulled = 0;
  result.maxDepthVisited = 0;

  Tile* pRootTile = this->getRootTile();
  if (!pRootTile) {
    return result;
  }

  if (!this->supportsRasterOverlays() && this->_overlays.size() > 0) {
    this->_externals.pLogger->warn(
        "Only quantized-mesh terrain tilesets currently support overlays.");
  }

  this->_loadQueueHigh.clear();
  this->_loadQueueMedium.clear();
  this->_loadQueueLow.clear();
  this->_subtreeLoadQueue.clear();

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
    this->_visitTileIfNeeded(
        frameState,
        ImplicitTraversalInfo(pRootTile),
        0,
        false,
        *pRootTile,
        result);
  } else {
    result = ViewUpdateResult();
  }

  result.tilesLoadingLowPriority =
      static_cast<uint32_t>(this->_loadQueueLow.size());
  result.tilesLoadingMediumPriority =
      static_cast<uint32_t>(this->_loadQueueMedium.size());
  result.tilesLoadingHighPriority =
      static_cast<uint32_t>(this->_loadQueueHigh.size());

  this->_unloadCachedTiles();
  this->_processLoadQueue();

  // aggregate all the credits needed from this tileset for the current frame
  const std::shared_ptr<CreditSystem>& pCreditSystem =
      this->_externals.pCreditSystem;
  if (pCreditSystem && !result.tilesToRenderThisFrame.empty()) {
    // per-tileset user-specified credit
    if (this->_userCredit) {
      pCreditSystem->addCreditToFrame(this->_userCredit.value());
    }

    // per-raster overlay credit
    for (auto& pOverlay : this->_overlays) {
      const std::optional<Credit>& overlayCredit =
          pOverlay->getTileProvider()->getCredit();
      if (overlayCredit) {
        pCreditSystem->addCreditToFrame(overlayCredit.value());
      }
    }

    // per-tile credits
    for (auto& tile : result.tilesToRenderThisFrame) {
      const std::vector<RasterMappedTo3DTile>& mappedRasterTiles =
          tile->getMappedRasterTiles();
      for (const RasterMappedTo3DTile& mappedRasterTile : mappedRasterTiles) {
        const RasterOverlayTile* pRasterOverlayTile =
            mappedRasterTile.getReadyTile();
        if (pRasterOverlayTile != nullptr) {
          for (const Credit& credit : pRasterOverlayTile->getCredits()) {
            pCreditSystem->addCreditToFrame(credit);
          }
        }
      }
      if (tile->getContent() != nullptr) {
        for (const Credit& credit : tile->getContent()->credits) {
          pCreditSystem->addCreditToFrame(credit);
        }
      }
      if (tile->getContext()->implicitContext &&
          tile->getContext()->implicitContext->credit) {
        pCreditSystem->addCreditToFrame(
            *tile->getContext()->implicitContext->credit);
      }
    }
  }

  this->_previousFrameNumber = currentFrameNumber;

  return result;
}

void Tileset::notifyTileStartLoading(Tile* pTile) noexcept {
  ++this->_loadsInProgress;

  if (pTile) {
    CESIUM_TRACE_BEGIN_IN_TRACK(
        TileIdUtilities::createTileIdString(pTile->getTileID()).c_str());
  }
}

void Tileset::notifyTileDoneLoading(Tile* pTile) noexcept {
  assert(this->_loadsInProgress > 0);
  --this->_loadsInProgress;

  if (pTile) {
    this->_tileDataBytes += pTile->computeByteSize();

    CESIUM_TRACE_END_IN_TRACK(
        TileIdUtilities::createTileIdString(pTile->getTileID()).c_str());
  }
}

void Tileset::notifyTileUnloading(Tile* pTile) noexcept {
  if (pTile) {
    this->_tileDataBytes -= pTile->computeByteSize();
  }
}

void Tileset::loadTilesFromJson(
    Tile& rootTile,
    std::vector<std::unique_ptr<TileContext>>& newContexts,
    const rapidjson::Value& tilesetJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    const TileContext& context,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  LoadTileFromJson::execute(
      rootTile,
      newContexts,
      tilesetJson["root"],
      parentTransform,
      parentRefine,
      context,
      pLogger);
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
Tileset::requestTileContent(Tile& tile) {
  std::string url =
      this->getResolvedContentUrl(*tile.getContext(), tile.getTileID());
  assert(!url.empty());

  this->notifyTileStartLoading(&tile);

  return this->getExternals().pAssetAccessor->get(
      this->getAsyncSystem(),
      url,
      tile.getContext()->requestHeaders);
}

CesiumAsync::SharedFuture<int> Tileset::requestAvailabilityTile(
    const CesiumGeometry::QuadtreeTileID& availabilityTileID,
    TileContext* pAvailabilityContext) {
  AvailabilityLoadRecord record{
      availabilityTileID,
      pAvailabilityContext,
      _asyncSystem.createResolvedFuture(0).share()};
  auto recordIt = std::find(
      _availabilityLoading.begin(),
      _availabilityLoading.end(),
      record);
  if (recordIt != _availabilityLoading.end()) {
    return recordIt->future;
  }

  std::string url =
      getResolvedContentUrl(*pAvailabilityContext, availabilityTileID);
  record.future =
      _externals.pAssetAccessor
          ->get(
              _externals.asyncSystem,
              url,
              pAvailabilityContext->requestHeaders)
          .thenInWorkerThread(
              [pLogger = _externals.pLogger, url, availabilityTileID](
                  std::shared_ptr<IAssetRequest>&& pRequest)
                  -> std::vector<QuadtreeTileRectangularRange> {
                const IAssetResponse* pResponse = pRequest->response();
                if (pResponse) {
                  uint16_t statusCode = pResponse->statusCode();

                  if (!(statusCode != 0 &&
                        (statusCode < 200 || statusCode >= 300))) {
                    return QuantizedMeshContent::loadMetadata(
                        pLogger,
                        pResponse->data(),
                        availabilityTileID);
                  }
                }
                return {};
              })
          .thenInMainThread(
              [pAvailabilityContext, availabilityTileID, this](
                  std::vector<CesiumGeometry::QuadtreeTileRectangularRange>&&
                      rectangles) {
                pAvailabilityContext->implicitContext->availabilityTilesLoaded
                    .insert(availabilityTileID);
                if (!rectangles.empty()) {
                  for (const QuadtreeTileRectangularRange& range : rectangles) {
                    pAvailabilityContext->implicitContext->rectangleAvailability
                        ->addAvailableTileRange(range);
                  }
                }
                AvailabilityLoadRecord record{
                    availabilityTileID,
                    pAvailabilityContext,
                    _asyncSystem.createResolvedFuture(0).share()};
                auto recordIt = std::find(
                    _availabilityLoading.begin(),
                    _availabilityLoading.end(),
                    record);
                if (recordIt != _availabilityLoading.end()) {
                  _availabilityLoading.erase(recordIt);
                }
                return 0;
              })
          .share();
  _availabilityLoading.push_back(record);
  return record.future;
}

void Tileset::addContext(std::unique_ptr<TileContext>&& pNewContext) {
  this->_contexts.push_back(std::move(pNewContext));
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

int64_t Tileset::getTotalDataBytes() const noexcept {
  int64_t bytes = this->_tileDataBytes;

  for (auto& pOverlay : this->_overlays) {
    const RasterOverlayTileProvider* pProvider = pOverlay->getTileProvider();
    if (pProvider) {
      bytes += pProvider->getTileDataBytes();
    }
  }

  return bytes;
}

static void markTileNonRendered(
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Rendered) {
    result.tilesToNoLongerRenderThisFrame.push_back(&tile);
  }
}

static void markTileNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markTileNonRendered(lastResult, tile, result);
}

static void markChildrenNonRendered(
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

static void markChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

static void markTileAndChildrenNonRendered(
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
static bool isVisibleFromCamera(
    const ViewState& viewState,
    const BoundingVolume& boundingVolume,
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
      estimateGlobeRectangle(boundingVolume);
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
static bool isVisibleInFog(double distance, double fogDensity) noexcept {
  if (fogDensity <= 0.0) {
    return true;
  }

  const double fogScalar = distance * fogDensity;
  return glm::exp(-(fogScalar * fogScalar)) > 0.0;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * It is not yet known whether the tile is visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTileIfNeeded(
    const FrameState& frameState,
    ImplicitTraversalInfo implicitInfo,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {

  if (tile.getState() == Tile::LoadState::ContentLoaded) {
    tile.processLoadedContent();
    ImplicitTraversalUtilities::createImplicitChildrenIfNeeded(
        tile,
        implicitInfo);
  }

  tile.update(frameState.lastFrameNumber, frameState.currentFrameNumber);

  this->_markTileVisited(tile);

  // whether we should visit this tile
  bool shouldVisit = true;
  // whether this tile was culled (Note: we might still want to visit it)
  bool culled = false;

  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       this->_options.excluders) {
    if (pExcluder->shouldExclude(tile)) {
      culled = true;
      shouldVisit = false;
      break;
    }
  }

  const std::vector<ViewState>& frustums = frameState.frustums;
  const std::vector<double>& fogDensities = frameState.fogDensities;

  const BoundingVolume& boundingVolume = tile.getBoundingVolume();
  if (std::none_of(
          frustums.begin(),
          frustums.end(),
          [boundingVolume,
           renderTilesUnderCamera = this->_options.renderTilesUnderCamera](
              const ViewState& frustum) {
            return isVisibleFromCamera(
                frustum,
                boundingVolume,
                renderTilesUnderCamera);
          })) {
    // this tile is off-screen so it is a culled tile
    culled = true;
    if (this->_options.enableFrustumCulling) {
      // frustum culling is enabled so we shouldn't visit this off-screen tile
      shouldVisit = false;
    }
  }

  if (this->_nextDistancesVector >= this->_distancesStack.size()) {
    this->_distancesStack.resize(this->_nextDistancesVector + 1);
  }

  std::unique_ptr<std::vector<double>>& pDistances =
      this->_distancesStack[this->_nextDistancesVector];
  if (!pDistances) {
    pDistances = std::make_unique<std::vector<double>>();
  }

  std::vector<double>& distances = *pDistances;
  distances.resize(frustums.size());
  ++this->_nextDistancesVector;

  // Use a ScopeGuard to ensure the _nextDistancesVector gets decrements when we
  // leave this scope.
  CesiumUtility::ScopeGuard guard{[this]() { --this->_nextDistancesVector; }};

  std::transform(
      frustums.begin(),
      frustums.end(),
      distances.begin(),
      [boundingVolume](const ViewState& frustum) -> double {
        return glm::sqrt(glm::max(
            frustum.computeDistanceSquaredToBoundingVolume(boundingVolume),
            0.0));
      });

  // if we are still considering visiting this tile, check for fog occlusion
  if (shouldVisit) {
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
      culled = true;
      if (this->_options.enableFogCulling) {
        // fog culling is enabled so we shouldn't visit this tile
        shouldVisit = false;
      }
    }
  }

  if (!shouldVisit) {
    markTileAndChildrenNonRendered(frameState.lastFrameNumber, tile, result);
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Culled));

    // Preload this culled sibling if requested.
    if (this->_options.preloadSiblings) {
      addTileToLoadQueue(
          this->_loadQueueLow,
          implicitInfo,
          frustums,
          tile,
          distances);
    }

    ++result.tilesCulled;

    return TraversalDetails();
  }

  return this->_visitTile(
      frameState,
      implicitInfo,
      depth,
      ancestorMeetsSse,
      tile,
      distances,
      culled,
      result);
}

static bool isLeaf(const Tile& tile) noexcept {
  return tile.getChildren().empty();
}

Tileset::TraversalDetails Tileset::_renderLeaf(
    const FrameState& frameState,
    const ImplicitTraversalInfo& implicitInfo,
    Tile& tile,
    const std::vector<double>& distances,
    ViewUpdateResult& result) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);

  double loadPriority = addTileToLoadQueue(
      this->_loadQueueMedium,
      implicitInfo,
      frameState.frustums,
      tile,
      distances);

  if (implicitInfo.shouldQueueSubtreeLoad) {
    this->addSubtreeToLoadQueue(tile, implicitInfo, loadPriority);
  }

  TraversalDetails traversalDetails;
  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  traversalDetails.notYetRenderableCount =
      traversalDetails.allAreRenderable ? 0 : 1;
  return traversalDetails;
}

bool Tileset::_queueLoadOfChildrenRequiredForForbidHoles(
    const FrameState& frameState,
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo,
    const std::vector<double>& distances) {
  // This method should only be called in "Forbid Holes" mode.
  assert(this->_options.forbidHoles);

  bool waitingForChildren = false;

  // If we're forbidding holes, don't refine if any children are still loading.
  gsl::span<Tile> children = tile.getChildren();
  for (Tile& child : children) {
    this->_markTileVisited(child);

    if (!child.isRenderable() && !child.isExternalTileset()) {
      waitingForChildren = true;

      ImplicitTraversalInfo childInfo(&child, &implicitInfo);

      // While we are waiting for the child to load, we need to push along the
      // tile and raster loading by continuing to update it.
      if (child.getState() == Tile::LoadState::ContentLoaded) {
        child.processLoadedContent();
        ImplicitTraversalUtilities::createImplicitChildrenIfNeeded(
            child,
            childInfo);
      }
      child.update(frameState.lastFrameNumber, frameState.currentFrameNumber);

      // We're using the distance to the parent tile to compute the load
      // priority. This is fine because the relative priority of the children is
      // irrelevant; we can't display any of them until all are loaded, anyway.
      addTileToLoadQueue(
          this->_loadQueueMedium,
          childInfo,
          frameState.frustums,
          child,
          distances);
    } else if (child.getUnconditionallyRefine()) {
      // This child tile is set to unconditionally refine. That means refining
      // _to_ it will immediately refine _through_ it. So we need to make sure
      // its children are renderable, too.
      // The distances are not correct for the child's children, but once again
      // we don't care because all tiles must be loaded before we can render any
      // of them, so their relative priority doesn't matter.
      ImplicitTraversalInfo childInfo(&child, &implicitInfo);
      waitingForChildren |= this->_queueLoadOfChildrenRequiredForForbidHoles(
          frameState,
          child,
          childInfo,
          distances);
    }
  }

  return waitingForChildren;
}

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

/**
 * We can render it if _any_ of the following are true:
 *  1. We rendered it (or kicked it) last frame.
 *  2. This tile was culled last frame, or it wasn't even visited because an
 * ancestor was culled.
 *  3. The tile is done loading and ready to render.
 *  Note that even if we decide to render a tile here, it may later get "kicked"
 * in favor of an ancestor.
 */
static bool shouldRenderThisTile(
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState,
    int32_t lastFrameNumber) noexcept {
  const TileSelectionState::Result originalResult =
      lastFrameSelectionState.getOriginalResult(lastFrameNumber);
  if (originalResult == TileSelectionState::Result::Rendered) {
    return true;
  }
  if (originalResult == TileSelectionState::Result::Culled ||
      originalResult == TileSelectionState::Result::None) {
    return true;
  }

  // Tile::isRenderable is actually a pretty complex operation, so only do
  // it when absolutely necessary
  if (tile.isRenderable()) {
    return true;
  }
  return false;
}

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

  TraversalDetails traversalDetails;
  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  traversalDetails.notYetRenderableCount =
      traversalDetails.allAreRenderable ? 0 : 1;

  return traversalDetails;
}

Tileset::TraversalDetails Tileset::_refineToNothing(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    bool areChildrenRenderable) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  // Nothing else to do except mark this tile refined and return.
  TraversalDetails noChildrenTraversalDetails;
  if (tile.getRefine() == TileRefine::Add) {
    noChildrenTraversalDetails.allAreRenderable = tile.isRenderable();
    noChildrenTraversalDetails.anyWereRenderedLastFrame =
        lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
        TileSelectionState::Result::Rendered;
    noChildrenTraversalDetails.notYetRenderableCount =
        areChildrenRenderable ? 0 : 1;
  } else {
    markTileNonRendered(frameState.lastFrameNumber, tile, result);
  }

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Refined));
  return noChildrenTraversalDetails;
}

bool Tileset::_loadAndRenderAdditiveRefinedTile(
    const FrameState& frameState,
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo,
    ViewUpdateResult& result,
    const std::vector<double>& distances) {
  // If this tile uses additive refinement, we need to render this tile in
  // addition to its children.
  if (tile.getRefine() == TileRefine::Add) {
    result.tilesToRenderThisFrame.push_back(&tile);
    addTileToLoadQueue(
        this->_loadQueueMedium,
        implicitInfo,
        frameState.frustums,
        tile,
        distances);
    return true;
  }

  return false;
}

// TODO This function is obviously too complex. The way how the indices are
// used, in order to deal with the queue elements, should be reviewed...
bool Tileset::_kickDescendantsAndRenderTile(
    const FrameState& frameState,
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo,
    ViewUpdateResult& result,
    TraversalDetails& traversalDetails,
    size_t firstRenderedDescendantIndex,
    size_t loadIndexLow,
    size_t loadIndexMedium,
    size_t loadIndexHigh,
    bool queuedForLoad,
    const std::vector<double>& distances) {
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
  const bool wasRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  const bool wasReallyRenderedLastFrame =
      wasRenderedLastFrame && tile.isRenderable();

  if (!wasReallyRenderedLastFrame &&
      traversalDetails.notYetRenderableCount >
          this->_options.loadingDescendantLimit) {
    // Remove all descendants from the load queues.
    this->_loadQueueLow.erase(
        this->_loadQueueLow.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexLow),
        this->_loadQueueLow.end());
    this->_loadQueueMedium.erase(
        this->_loadQueueMedium.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexMedium),
        this->_loadQueueMedium.end());
    this->_loadQueueHigh.erase(
        this->_loadQueueHigh.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexHigh),
        this->_loadQueueHigh.end());

    if (!queuedForLoad) {
      addTileToLoadQueue(
          this->_loadQueueMedium,
          implicitInfo,
          frameState.frustums,
          tile,
          distances);
    }

    traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
    queuedForLoad = true;
  }

  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame = wasRenderedLastFrame;

  return queuedForLoad;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * The tile has previously been determined to be visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTile(
    const FrameState& frameState,
    const ImplicitTraversalInfo& implicitInfo,
    uint32_t depth,
    bool ancestorMeetsSse, // Careful: May be modified before being passed to
                           // children!
    Tile& tile,
    const std::vector<double>& distances,
    bool culled,
    ViewUpdateResult& result) {
  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  if (culled) {
    ++result.culledTilesVisited;
  }

  // If this is a leaf tile, just render it (it's already been deemed visible).
  if (isLeaf(tile)) {
    return _renderLeaf(frameState, implicitInfo, tile, distances, result);
  }

  const bool unconditionallyRefine = tile.getUnconditionallyRefine();
  const bool meetsSse = _meetsSse(frameState.frustums, tile, distances, culled);

  bool wantToRefine = unconditionallyRefine || (!meetsSse && !ancestorMeetsSse);

  // In "Forbid Holes" mode, we cannot refine this tile until all its children
  // are loaded. But don't queue the children for load until we _want_ to
  // refine this tile.
  if (wantToRefine && this->_options.forbidHoles) {
    const bool waitingForChildren =
        this->_queueLoadOfChildrenRequiredForForbidHoles(
            frameState,
            tile,
            implicitInfo,
            distances);
    wantToRefine = !waitingForChildren;
  }

  if (!wantToRefine) {
    // This tile (or an ancestor) is the one we want to render this frame, but
    // we'll do different things depending on the state of this tile and on what
    // we did _last_ frame.

    // We can render it if _any_ of the following are true:
    // 1. We rendered it (or kicked it) last frame.
    // 2. This tile was culled last frame, or it wasn't even visited because an
    // ancestor was culled.
    // 3. The tile is done loading and ready to render.
    //
    // Note that even if we decide to render a tile here, it may later get
    // "kicked" in favor of an ancestor.
    const TileSelectionState lastFrameSelectionState =
        tile.getLastSelectionState();
    const bool renderThisTile = shouldRenderThisTile(
        tile,
        lastFrameSelectionState,
        frameState.lastFrameNumber);
    if (renderThisTile) {
      // Only load this tile if it (not just an ancestor) meets the SSE.
      if (meetsSse && !ancestorMeetsSse) {
        addTileToLoadQueue(
            this->_loadQueueMedium,
            implicitInfo,
            frameState.frustums,
            tile,
            distances);
      }
      return _renderInnerTile(frameState, tile, result);
    }

    // Otherwise, we can't render this tile (or blank space where it would be)
    // because doing so would cause detail to disappear that was visible last
    // frame. Instead, keep rendering any still-visible descendants that were
    // rendered last frame and render nothing for newly-visible descendants.
    // E.g. if we were rendering level 15 last frame but this frame we want
    // level 14 and the closest renderable level <= 14 is 0, rendering level
    // zero would be pretty jarring so instead we keep rendering level 15 even
    // though its SSE is better than required. So fall through to continue
    // traversal...
    ancestorMeetsSse = true;

    // Load this blocker tile with high priority, but only if this tile (not
    // just an ancestor) meets the SSE.
    if (meetsSse) {
      addTileToLoadQueue(
          this->_loadQueueHigh,
          implicitInfo,
          frameState.frustums,
          tile,
          distances);
    }
  }

  // Refine!

  bool queuedForLoad = _loadAndRenderAdditiveRefinedTile(
      frameState,
      tile,
      implicitInfo,
      result,
      distances);

  const size_t firstRenderedDescendantIndex =
      result.tilesToRenderThisFrame.size();
  const size_t loadIndexLow = this->_loadQueueLow.size();
  const size_t loadIndexMedium = this->_loadQueueMedium.size();
  const size_t loadIndexHigh = this->_loadQueueHigh.size();

  TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(
      frameState,
      implicitInfo,
      depth,
      ancestorMeetsSse,
      tile,
      result);

  const bool descendantTilesAdded =
      firstRenderedDescendantIndex != result.tilesToRenderThisFrame.size();
  if (!descendantTilesAdded) {
    // No descendant tiles were added to the render list by the function above,
    // meaning they were all culled even though this tile was deemed visible.
    // That's pretty common.
    return _refineToNothing(
        frameState,
        tile,
        result,
        traversalDetails.allAreRenderable);
  }

  // At least one descendant tile was added to the render list.
  // The traversalDetails tell us what happened while visiting the children.
  if (!traversalDetails.allAreRenderable &&
      !traversalDetails.anyWereRenderedLastFrame) {
    // Some of our descendants aren't ready to render yet, and none were
    // rendered last frame, so kick them all out of the render list and render
    // this tile instead. Continue to load them though!
    queuedForLoad = _kickDescendantsAndRenderTile(
        frameState,
        tile,
        implicitInfo,
        result,
        traversalDetails,
        firstRenderedDescendantIndex,
        loadIndexLow,
        loadIndexMedium,
        loadIndexHigh,
        queuedForLoad,
        distances);
  } else {
    if (tile.getRefine() != TileRefine::Add) {
      markTileNonRendered(frameState.lastFrameNumber, tile, result);
    }
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Refined));
  }

  if (this->_options.preloadAncestors && !queuedForLoad) {
    addTileToLoadQueue(
        this->_loadQueueLow,
        implicitInfo,
        frameState.frustums,
        tile,
        distances);
  }

  return traversalDetails;
}

Tileset::TraversalDetails Tileset::_visitVisibleChildrenNearToFar(
    const FrameState& frameState,
    const ImplicitTraversalInfo& implicitInfo,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {
  TraversalDetails traversalDetails;

  // TODO: actually visit near-to-far, rather than in order of occurrence.
  gsl::span<Tile> children = tile.getChildren();
  for (Tile& child : children) {
    const TraversalDetails childTraversal = this->_visitTileIfNeeded(
        frameState,
        ImplicitTraversalInfo(&child, &implicitInfo),
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

void Tileset::_processLoadQueue() {
  this->processQueue(
      this->_loadQueueHigh,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  this->processQueue(
      this->_loadQueueMedium,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  this->processQueue(
      this->_loadQueueLow,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  this->processSubtreeQueue();
}

void Tileset::_unloadCachedTiles() noexcept {
  const int64_t maxBytes = this->getOptions().maximumCachedBytes;

  Tile* pTile = this->_loadedTiles.head();

  while (this->getTotalDataBytes() > maxBytes) {
    if (pTile == nullptr || pTile == this->_pRootTile.get()) {
      // We've either removed all tiles or the next tile is the root.
      // The root tile marks the beginning of the tiles that were used
      // for rendering last frame.
      break;
    }

    Tile* pNext = this->_loadedTiles.next(*pTile);

    const bool removed = pTile->unloadContent();
    if (removed) {
      this->_loadedTiles.remove(*pTile);
    }

    pTile = pNext;
  }
}

void Tileset::_markTileVisited(Tile& tile) noexcept {
  this->_loadedTiles.insertAtTail(tile);
}

std::string Tileset::getResolvedContentUrl(
    const TileContext& context,
    const TileID& tileID) const {
  struct Operation {
    const TileContext& context;

    std::string operator()(const std::string& url) { return url; }

    std::string operator()(const QuadtreeTileID& quadtreeID) {
      if (!this->context.implicitContext) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          context.implicitContext.value().tileTemplateUrls[0],
          [this, &quadtreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level" || placeholder == "z") {
              return std::to_string(quadtreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(quadtreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(quadtreeID.y);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(const OctreeTileID& octreeID) {
      if (!this->context.implicitContext) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          context.implicitContext.value().tileTemplateUrls[0],
          [this, &octreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level") {
              return std::to_string(octreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(octreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(octreeID.y);
            }
            if (placeholder == "z") {
              return std::to_string(octreeID.z);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string
    operator()(UpsampledQuadtreeNode /*subdividedParent*/) noexcept {
      return std::string();
    }
  };

  std::string url = std::visit(Operation{context}, tileID);
  if (url.empty()) {
    return url;
  }

  return CesiumUtility::Uri::resolve(context.baseUrl, url, true);
}

static bool anyRasterOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    const RasterOverlayTile* pLoading = mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() == RasterOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

// TODO The viewState is only needed to
// compute the priority from the distance. So maybe this function should
// receive a priority directly and be called with
// addTileToLoadQueue(queue, tile, priorityFor(tile, viewState, distance))
// (or at least, this function could delegate to such a call...)

/*static*/ double Tileset::addTileToLoadQueue(
    std::vector<Tileset::LoadRecord>& loadQueue,
    const ImplicitTraversalInfo& implicitInfo,
    const std::vector<ViewState>& frustums,
    Tile& tile,
    const std::vector<double>& distances) {
  double highestLoadPriority = std::numeric_limits<double>::max();

  if (tile.getState() == Tile::LoadState::Unloaded ||
      anyRasterOverlaysNeedLoading(tile)) {

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

    // Check if the tile has any content
    const std::string* pStringID = std::get_if<std::string>(&tile.getTileID());
    const bool emptyContentUri = pStringID && pStringID->empty();
    const bool usingImplicitTiling = implicitInfo.usingImplicitQuadtreeTiling ||
                                     implicitInfo.usingImplicitOctreeTiling;
    const bool subtreeLoaded =
        implicitInfo.pCurrentNode && implicitInfo.pCurrentNode->subtree;
    const bool implicitContentAvailability =
        implicitInfo.availability & TileAvailabilityFlags::CONTENT_AVAILABLE;

    bool shouldLoad = false;
    bool hasNoContent = false;

    if (usingImplicitTiling) {
      if (subtreeLoaded) {
        if (implicitContentAvailability) {
          shouldLoad = true;
        } else {
          hasNoContent = true;
        }
      }

      // Note: We do nothing if we don't _know_ the content availability yet,
      // i.e., the subtree isn't loaded.
    } else {
      if (emptyContentUri) {
        hasNoContent = true;
      } else {
        // Assume it has loadable content.
        shouldLoad = true;
      }
    }

    if (hasNoContent) {
      // The tile doesn't have content, so just put it in the ContentLoaded
      // state if needed.
      if (tile.getState() == Tile::LoadState::Unloaded) {
        tile.setState(Tile::LoadState::ContentLoaded);

        // There are two possible ways to handle a tile with no content:
        //
        // 1. Treat it as a placeholder used for more efficient culling, but
        //    never render it. Refining to this tile is equivalent to refining
        //    to its children. To have this behavior, the tile _should_ have
        //    content, but that content's model should be std::nullopt.
        // 2. Treat it as an indication that nothing need be rendered in this
        //    area at this level-of-detail. In other words, "render" it as a
        //    hole. To have this behavior, the tile should _not_ have content at
        //    all.
        //
        // We distinguish whether the tileset creator wanted (1) or (2) by
        // comparing this tile's geometricError to the geometricError of its
        // parent tile. If this tile's error is greater than or equal to its
        // parent, treat it as (1). If it's less, treat it as (2).
        //
        // For a tile with no parent there's no difference between the
        // behaviors.
        double myGeometricError = tile.getNonZeroGeometricError();

        Tile* pAncestor = tile.getParent();
        while (pAncestor && pAncestor->getUnconditionallyRefine()) {
          pAncestor = pAncestor->getParent();
        }

        double parentGeometricError =
            pAncestor ? pAncestor->getNonZeroGeometricError()
                      : myGeometricError * 2.0;
        if (myGeometricError >= parentGeometricError) {
          tile.setEmptyContent();
        }
      }
    } else if (shouldLoad) {
      loadQueue.push_back({&tile, highestLoadPriority});
    }
  }

  return highestLoadPriority;
}

void Tileset::processQueue(
    std::vector<Tileset::LoadRecord>& queue,
    const std::atomic<uint32_t>& loadsInProgress,
    uint32_t maximumLoadsInProgress) {
  if (loadsInProgress >= maximumLoadsInProgress) {
    return;
  }

  std::sort(queue.begin(), queue.end());

  for (LoadRecord& record : queue) {
    CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
    record.pTile->loadContent();
    if (loadsInProgress >= maximumLoadsInProgress) {
      break;
    }
  }
}

void Tileset::addSubtreeToLoadQueue(
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo,
    double loadPriority) {

  if (!implicitInfo.pCurrentNode &&
      (implicitInfo.availability & TileAvailabilityFlags::SUBTREE_AVAILABLE) &&
      implicitInfo.shouldQueueSubtreeLoad &&
      (implicitInfo.usingImplicitQuadtreeTiling ||
       implicitInfo.usingImplicitOctreeTiling) &&
      !implicitInfo.pCurrentNode) {

    this->_subtreeLoadQueue.push_back({&tile, implicitInfo, loadPriority});
  }
}

void Tileset::processSubtreeQueue() {
  if (this->_subtreeLoadsInProgress >=
      this->_options.maximumSimultaneousSubtreeLoads) {
    return;
  }

  std::sort(this->_subtreeLoadQueue.begin(), this->_subtreeLoadQueue.end());

  for (SubtreeLoadRecord& record : this->_subtreeLoadQueue) {
    // TODO: tracing code here
    ++this->_subtreeLoadsInProgress;
    LoadSubtree::start(*this, record).thenInMainThread([this]() {
      --this->_subtreeLoadsInProgress;
    });
    if (this->_subtreeLoadsInProgress >=
        this->_options.maximumSimultaneousSubtreeLoads) {
      break;
    }
  }
}

void Tileset::reportError(TilesetLoadFailureDetails&& errorDetails) {
  SPDLOG_LOGGER_ERROR(this->getExternals().pLogger, errorDetails.message);
  if (this->getOptions().loadErrorCallback) {
    this->getExternals().asyncSystem.runInMainThread(
        [this, errorDetails = std::move(errorDetails)]() mutable {
          this->getOptions().loadErrorCallback(std::move(errorDetails));
        });
  }
}

} // namespace Cesium3DTilesSelection
