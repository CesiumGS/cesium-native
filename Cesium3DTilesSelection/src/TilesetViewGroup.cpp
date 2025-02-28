#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <CesiumUtility/Assert.h>

namespace Cesium3DTilesSelection {

TilesetViewGroup::TilesetViewGroup(const TilesetViewGroup& rhs) noexcept =
    default;

TilesetViewGroup::TilesetViewGroup(TilesetViewGroup&& rhs) noexcept = default;

TilesetViewGroup::~TilesetViewGroup() noexcept = default;

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
}

TilesetViewGroup::TilesetViewGroup(
    const CesiumUtility::IntrusivePointer<TilesetContentManager>&
        pTilesetContentManager)
    : _pTilesetContentManager(pTilesetContentManager),
      _previousSelectionStates(),
      _currentSelectionStates() {}

} // namespace Cesium3DTilesSelection
