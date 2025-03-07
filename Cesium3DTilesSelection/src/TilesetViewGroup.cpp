#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <CesiumUtility/Assert.h>

namespace Cesium3DTilesSelection {

TilesetViewGroup::TilesetViewGroup(const TilesetViewGroup& rhs) noexcept
    : _pTilesetContentManager(rhs._pTilesetContentManager),
      _weight(rhs._weight),
      _previousSelectionStates(rhs._previousSelectionStates),
      _currentSelectionStates(rhs._currentSelectionStates),
      _mainThreadLoadQueue(rhs._mainThreadLoadQueue),
      _workerThreadLoadQueue(rhs._workerThreadLoadQueue) {
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->registerTileRequester(*this);
  }
}

TilesetViewGroup::TilesetViewGroup(TilesetViewGroup&& rhs) noexcept
    : _pTilesetContentManager(rhs._pTilesetContentManager),
      _weight(rhs._weight),
      _previousSelectionStates(std::move(rhs._previousSelectionStates)),
      _currentSelectionStates(std::move(rhs._currentSelectionStates)),
      _mainThreadLoadQueue(std::move(rhs._mainThreadLoadQueue)),
      _workerThreadLoadQueue(std::move(rhs._workerThreadLoadQueue)) {
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->registerTileRequester(*this);
  }
}

TilesetViewGroup::~TilesetViewGroup() noexcept {
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->unregisterTileRequester(*this);
  }
}

TileSelectionState
TilesetViewGroup::getPreviousSelectionState(const Tile& tile) const noexcept {
  auto it = this->_previousSelectionStates.find(&tile);
  if (it == this->_previousSelectionStates.end()) {
    return TileSelectionState();
  } else {
    return it->second;
  }
}

TileSelectionState
TilesetViewGroup::getCurrentSelectionState(const Tile& tile) const noexcept {
  auto it = this->_currentSelectionStates.find(&tile);
  if (it == this->_currentSelectionStates.end()) {
    return TileSelectionState();
  } else {
    return it->second;
  }
}

void TilesetViewGroup::setCurrentSelectionState(
    const Tile& tile,
    const TileSelectionState& newState) noexcept {
  this->_currentSelectionStates[&tile] = newState;
}

void TilesetViewGroup::kick(const Tile& tile) noexcept {
  auto it = this->_currentSelectionStates.find(&tile);
  if (it == this->_currentSelectionStates.end()) {
    // There should already be a selection result for this tile prior to
    // kicking.
    CESIUM_ASSERT(false);
    return;
  } else {
    it->second.kick();
  }
}

void TilesetViewGroup::finishFrame() {
  std::swap(this->_previousSelectionStates, this->_currentSelectionStates);
  this->_currentSelectionStates.clear();

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

TilesetViewGroup::TilesetViewGroup(
    const CesiumUtility::IntrusivePointer<TilesetContentManager>&
        pTilesetContentManager)
    : _pTilesetContentManager(pTilesetContentManager),
      _weight(1.0),
      _previousSelectionStates(),
      _currentSelectionStates(),
      _mainThreadLoadQueue(),
      _workerThreadLoadQueue() {
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->registerTileRequester(*this);
  }
}

} // namespace Cesium3DTilesSelection
