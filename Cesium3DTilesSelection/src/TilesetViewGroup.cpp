#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumUtility/Assert.h>

#include <algorithm>
#include <cstddef>

namespace Cesium3DTilesSelection {

TilesetViewGroup::TilesetViewGroup() noexcept = default;

TilesetViewGroup::TilesetViewGroup(const TilesetViewGroup& rhs) noexcept =
    default;

TilesetViewGroup::TilesetViewGroup(TilesetViewGroup&& rhs) noexcept = default;

TilesetViewGroup::~TilesetViewGroup() noexcept = default;

const ViewUpdateResult& TilesetViewGroup::getViewUpdateResult() const {
  return this->_updateResult;
}

/** @copydoc getViewUpdateResult */
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
    ++this->_tilesAlreadyLoading;
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

void TilesetViewGroup::startNewFrame() {
  this->_workerThreadLoadQueue.clear();
  this->_mainThreadLoadQueue.clear();
  this->_tilesAlreadyLoading = 0;
  this->_traversalState.beginTraversal();
}

void TilesetViewGroup::finishFrame() {
  std::sort(
      this->_workerThreadLoadQueue.begin(),
      this->_workerThreadLoadQueue.end());

  // TODO: reverse the sort order instead?
  std::reverse(
      this->_workerThreadLoadQueue.begin(),
      this->_workerThreadLoadQueue.end());

  std::sort(
      this->_mainThreadLoadQueue.begin(),
      this->_mainThreadLoadQueue.end());

  // TODO: reverse the sort order instead?
  std::reverse(
      this->_mainThreadLoadQueue.begin(),
      this->_mainThreadLoadQueue.end());

  this->_updateResult.workerThreadTileLoadQueueLength =
      static_cast<int32_t>(this->getWorkerThreadLoadQueueLength());
  this->_updateResult.mainThreadTileLoadQueueLength =
      static_cast<int32_t>(this->getMainThreadLoadQueueLength());

  size_t totalTiles = this->_traversalState.getNodeCountInCurrentTraversal();
  size_t tilesLoading =
      size_t(this->_updateResult.workerThreadTileLoadQueueLength) +
      size_t(this->_updateResult.mainThreadTileLoadQueueLength) +
      this->_updateResult.tilesKicked + this->_tilesAlreadyLoading;

  if (tilesLoading == 0) {
    this->_loadProgressPercentage = 100.0f;
  } else {
    this->_loadProgressPercentage =
        100.0f * float(totalTiles - tilesLoading) / float(totalTiles);
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
