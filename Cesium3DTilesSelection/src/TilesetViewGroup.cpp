#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetFrameState.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

TilesetViewGroup::TilesetViewGroup() noexcept = default;

TilesetViewGroup::TilesetViewGroup(const TilesetViewGroup& rhs) noexcept =
    default;

TilesetViewGroup::TilesetViewGroup(TilesetViewGroup&& rhs) noexcept = default;

TilesetViewGroup::~TilesetViewGroup() noexcept = default;

const ViewUpdateResult& TilesetViewGroup::getViewUpdateResult() const {
  return this->_updateResult;
}

ViewUpdateResult& TilesetViewGroup::getViewUpdateResult() {
  return this->_updateResult;
}

void TilesetViewGroup::addToLoadQueue(const TileLoadTask& task) {
  Tile* pTile = task.pTile;
  CESIUM_ASSERT(pTile != nullptr);

  // Assert that this tile hasn't been added to a queue already.
  CESIUM_ASSERT(
      std::find_if(
          this->_workerThreadLoadQueue.begin(),
          this->_workerThreadLoadQueue.end(),
          [&](const TileLoadTask& task) { return task.pTile == pTile; }) ==
      this->_workerThreadLoadQueue.end());
  CESIUM_ASSERT(
      std::find_if(
          this->_mainThreadLoadQueue.begin(),
          this->_mainThreadLoadQueue.end(),
          [&](const TileLoadTask& task) { return task.pTile == pTile; }) ==
      this->_mainThreadLoadQueue.end());

  if (pTile->needsWorkerThreadLoading()) {
    this->_workerThreadLoadQueue.emplace_back(task);
  } else if (pTile->needsMainThreadLoading()) {
    this->_mainThreadLoadQueue.emplace_back(task);
  } else if (
      pTile->getState() == TileLoadState::ContentLoading ||
      pTile->getState() == TileLoadState::Unloading) {
    // These tiles are already transitioning between states so we can't add them
    // to either load queue. But they still count as tiles that need to be
    // loaded before this view is 100% loaded.
    ++this->_tilesAlreadyLoadingOrUnloading;
  }
}

TilesetViewGroup::LoadQueueCheckpoint
TilesetViewGroup::saveTileLoadQueueCheckpoint() {
  LoadQueueCheckpoint result;
  result.mainThread = this->_mainThreadLoadQueue.size();
  result.workerThread = this->_workerThreadLoadQueue.size();
  return result;
}

size_t TilesetViewGroup::restoreTileLoadQueueCheckpoint(
    const LoadQueueCheckpoint& checkpoint) {
  CESIUM_ASSERT(this->_workerThreadLoadQueue.size() >= checkpoint.workerThread);
  CESIUM_ASSERT(this->_mainThreadLoadQueue.size() >= checkpoint.mainThread);

  size_t before =
      this->_workerThreadLoadQueue.size() + this->_mainThreadLoadQueue.size();

  this->_workerThreadLoadQueue.resize(checkpoint.workerThread);
  this->_mainThreadLoadQueue.resize(checkpoint.mainThread);

  size_t after =
      this->_workerThreadLoadQueue.size() + this->_mainThreadLoadQueue.size();

  return before - after;
}

size_t TilesetViewGroup::getWorkerThreadLoadQueueLength() const {
  return this->_workerThreadLoadQueue.size();
}

size_t TilesetViewGroup::getMainThreadLoadQueueLength() const {
  return this->_mainThreadLoadQueue.size();
}

void TilesetViewGroup::startNewFrame(
    const Tileset& tileset,
    const TilesetFrameState& /*frameState*/) {
  this->_workerThreadLoadQueue.clear();
  this->_mainThreadLoadQueue.clear();
  this->_tilesAlreadyLoadingOrUnloading = 0;
  this->_traversalState.beginTraversal();

  ++this->_updateResult.frameNumber;
  this->_updateResult.tilesVisited = 0;
  this->_updateResult.culledTilesVisited = 0;
  this->_updateResult.tilesCulled = 0;
  this->_updateResult.tilesOccluded = 0;
  this->_updateResult.tilesWaitingForOcclusionResults = 0;
  this->_updateResult.tilesKicked = 0;
  this->_updateResult.maxDepthVisited = 0;

  this->_updateResult.tilesToRenderThisFrame.clear();

  if (!tileset.getOptions().enableLodTransitionPeriod) {
    this->_updateResult.tilesFadingOut.clear();
  }
}

void TilesetViewGroup::finishFrame(
    const Tileset& tileset,
    const TilesetFrameState& /*frameState*/) {
  std::sort(
      this->_workerThreadLoadQueue.begin(),
      this->_workerThreadLoadQueue.end());
  std::sort(
      this->_mainThreadLoadQueue.begin(),
      this->_mainThreadLoadQueue.end());

  ViewUpdateResult& updateResult = this->_updateResult;

  updateResult.workerThreadTileLoadQueueLength =
      static_cast<int32_t>(this->getWorkerThreadLoadQueueLength());
  updateResult.mainThreadTileLoadQueueLength =
      static_cast<int32_t>(this->getMainThreadLoadQueueLength());

  size_t totalTiles = this->_traversalState.getNodeCountInCurrentTraversal();
  size_t tilesLoading = size_t(updateResult.workerThreadTileLoadQueueLength) +
                        size_t(updateResult.mainThreadTileLoadQueueLength) +
                        updateResult.tilesKicked +
                        this->_tilesAlreadyLoadingOrUnloading;

  if (tilesLoading == 0) {
    this->_loadProgressPercentage = 100.0f;
  } else {
    this->_loadProgressPercentage =
        100.0f * float(totalTiles - tilesLoading) / float(totalTiles);
  }

  // aggregate all the credits needed from this tileset for the current frame
  const std::shared_ptr<CreditSystem>& pCreditSystem =
      tileset.getExternals().pCreditSystem;
  if (pCreditSystem) {
    this->_currentFrameCredits.setCreditSystem(pCreditSystem);

    // per-tileset user-specified credit
    std::optional<Credit> userCredit = tileset.getUserCredit();
    if (userCredit) {
      this->_currentFrameCredits.addCreditReference(*userCredit);
    }

    // tileset credit
    for (const Credit& credit : tileset.getTilesetCredits()) {
      this->_currentFrameCredits.addCreditReference(credit);
    }

    // per-raster overlay credit
    const RasterOverlayCollection& overlayCollection = tileset.getOverlays();
    for (auto& pTileProvider : overlayCollection.getTileProviders()) {
      const std::optional<Credit>& overlayCredit = pTileProvider->getCredit();
      if (overlayCredit) {
        this->_currentFrameCredits.addCreditReference(overlayCredit.value());
      }
    }

    // Add per-tile credits for tiles selected this frame.
    for (const Tile::Pointer& pTile : updateResult.tilesToRenderThisFrame) {
      const std::vector<RasterMappedTo3DTile>& mappedRasterTiles =
          pTile->getMappedRasterTiles();
      // raster overlay tile credits
      for (const RasterMappedTo3DTile& mappedRasterTile : mappedRasterTiles) {
        const RasterOverlayTile* pRasterOverlayTile =
            mappedRasterTile.getReadyTile();
        if (pRasterOverlayTile != nullptr) {
          for (const Credit& credit : pRasterOverlayTile->getCredits()) {
            this->_currentFrameCredits.addCreditReference(credit);
          }
        }
      }

      // content credits like gltf copyrights
      const TileRenderContent* pRenderContent =
          pTile->getContent().getRenderContent();
      if (pRenderContent) {
        for (const Credit& credit : pRenderContent->getCredits()) {
          this->_currentFrameCredits.addCreditReference(credit);
        }
      }
    }

    this->_previousFrameCredits.releaseAllReferences();
    std::swap(this->_previousFrameCredits, this->_currentFrameCredits);
  }
}

float TilesetViewGroup::getPreviousLoadProgressPercentage() const {
  return this->_loadProgressPercentage;
}

double TilesetViewGroup::getWeight() const { return this->_weight; }

void TilesetViewGroup::setWeight(double weight) noexcept {
  this->_weight = weight;
}

bool TilesetViewGroup::hasMoreTilesToLoadInWorkerThread() const {
  return !this->_workerThreadLoadQueue.empty();
}

Tile* TilesetViewGroup::getNextTileToLoadInWorkerThread() {
  CESIUM_ASSERT(!this->_workerThreadLoadQueue.empty());
  if (this->_workerThreadLoadQueue.empty())
    return nullptr;

  Tile* pResult = this->_workerThreadLoadQueue.back().pTile;
  this->_workerThreadLoadQueue.pop_back();
  return pResult;
}

bool TilesetViewGroup::hasMoreTilesToLoadInMainThread() const {
  return !this->_mainThreadLoadQueue.empty();
}

Tile* TilesetViewGroup::getNextTileToLoadInMainThread() {
  CESIUM_ASSERT(!this->_mainThreadLoadQueue.empty());
  if (this->_mainThreadLoadQueue.empty())
    return nullptr;

  Tile* pResult = this->_mainThreadLoadQueue.back().pTile;
  this->_mainThreadLoadQueue.pop_back();
  return pResult;
}

} // namespace Cesium3DTilesSelection
