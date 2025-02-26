#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <CesiumUtility/Assert.h>

namespace Cesium3DTilesSelection {

TilesetViewGroup::TilesetViewGroup(const TilesetViewGroup& rhs) noexcept
    : _pTilesetContentManager(rhs._pTilesetContentManager),
      _previousSelectionStates(rhs._previousSelectionStates),
      _currentSelectionStates(rhs._currentSelectionStates) {
  for (const std::pair<const Tile*, TileSelectionState>& pair :
       this->_previousSelectionStates) {
    [[maybe_unused]] bool firstReference = pair.first->addViewGroupReference();
    CESIUM_ASSERT(!firstReference);
  }

  for (const std::pair<const Tile*, TileSelectionState>& pair :
       this->_currentSelectionStates) {
    [[maybe_unused]] bool firstReference = pair.first->addViewGroupReference();
    CESIUM_ASSERT(!firstReference);
  }
}

TilesetViewGroup::TilesetViewGroup(TilesetViewGroup&& rhs) noexcept
    : _pTilesetContentManager(std::move(rhs._pTilesetContentManager)),
      _previousSelectionStates(std::move(rhs._previousSelectionStates)),
      _currentSelectionStates(std::move(rhs._currentSelectionStates)) {}

TilesetViewGroup::~TilesetViewGroup() noexcept {
  for (const std::pair<const Tile*, TileSelectionState>& pair :
       this->_previousSelectionStates) {
    if (pair.first->releaseViewGroupReference()) {
      this->_pTilesetContentManager->markTileNowUnused(*pair.first);
    }
  }

  for (const std::pair<const Tile*, TileSelectionState>& pair :
       this->_currentSelectionStates) {
    if (pair.first->releaseViewGroupReference()) {
      this->_pTilesetContentManager->markTileNowUnused(*pair.first);
    }
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
  bool inserted =
      this->_currentSelectionStates.insert_or_assign(&tile, newState).second;
  if (inserted) {
    if (tile.addViewGroupReference()) {
      // Tile was previously unused.
      this->_pTilesetContentManager->markTileNowUsed(tile);
    }
  }
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
  // Remove references from last frame's tiles, marking them unused if
  // appropriate.
  for (const std::pair<const Tile*, TileSelectionState>& pair :
       this->_previousSelectionStates) {
    if (pair.first->releaseViewGroupReference()) {
      this->_pTilesetContentManager->markTileNowUnused(*pair.first);
    }
  }

  std::swap(this->_previousSelectionStates, this->_currentSelectionStates);
  this->_currentSelectionStates.clear();
}

TilesetViewGroup::TilesetViewGroup(
    const CesiumUtility::IntrusivePointer<TilesetContentManager>&
        pTilesetContentManager)
    : _pTilesetContentManager(pTilesetContentManager),
      _previousSelectionStates(),
      _currentSelectionStates() {}

} // namespace Cesium3DTilesSelection
