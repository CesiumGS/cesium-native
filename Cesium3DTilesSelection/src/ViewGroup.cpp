#include <Cesium3DTilesSelection/ViewGroup.h>
#include <CesiumUtility/Assert.h>

namespace Cesium3DTilesSelection {

TileSelectionState
ViewGroup::getPreviousSelectionState(const Tile& tile) const noexcept {
  auto it = this->_previousSelectionStates.find(&tile);
  if (it == this->_previousSelectionStates.end()) {
    return TileSelectionState();
  } else {
    return it->second;
  }
}

TileSelectionState
ViewGroup::getCurrentSelectionState(const Tile& tile) const noexcept {
  auto it = this->_currentSelectionStates.find(&tile);
  if (it == this->_currentSelectionStates.end()) {
    return TileSelectionState();
  } else {
    return it->second;
  }
}

void ViewGroup::setCurrentSelectionState(
    const Tile& tile,
    const TileSelectionState& newState) noexcept {
  this->_currentSelectionStates[&tile] = newState;
}

void ViewGroup::startNextFrame() {
  std::swap(this->_previousSelectionStates, this->_currentSelectionStates);
  this->_currentSelectionStates.clear();
}

void ViewGroup::kick(const Tile& tile) noexcept {
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

} // namespace Cesium3DTilesSelection
