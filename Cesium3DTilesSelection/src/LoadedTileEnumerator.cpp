#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {

void LoadedTileEnumerator::updateRootTile(const Tile* pRootTile) {
  this->_pRootTile = pRootTile;
}

LoadedTileEnumerator::const_iterator LoadedTileEnumerator::begin() const {
  return const_iterator(this->_pRootTile);
}

LoadedTileEnumerator::const_iterator LoadedTileEnumerator::end() const {
  return const_iterator(nullptr);
}

LoadedTileEnumerator::const_iterator::const_iterator(const Tile* pRootTile)
    : _traversalStack() {
  if (pRootTile) {
    this->_traversalStack.push_back(pRootTile);
  }
}

LoadedTileEnumerator::const_iterator&
LoadedTileEnumerator::const_iterator::operator++() {
  if (this->_traversalStack.empty())
    return *this;

  const Tile* pCurrent = this->_traversalStack.back();

  // See if we can traverse down into a child tile.
  std::span<const Tile> children = pCurrent->getChildren();
  for (const Tile& child : children) {
    if (child.getDoNotUnloadSubtreeCount() > 0 ||
        child.getState() != TileLoadState::Unloaded) {
      this->_traversalStack.push_back(&child);
      return *this;
    }
  }

  // The current tile has no relevant children, so the next
  // tile in the traversal is this tile's sibling, if any.
  while (true) {
    this->_traversalStack.pop_back();

    if (!this->_traversalStack.empty()) {
      const Tile* pParentOfCurrent = this->_traversalStack.back();
      std::span<const Tile> childrenOfParent = pParentOfCurrent->getChildren();

      CESIUM_ASSERT(!childrenOfParent.empty());
      CESIUM_ASSERT(pCurrent >= &childrenOfParent.front());

      for (++pCurrent; pCurrent <= &childrenOfParent.back(); ++pCurrent) {
        if (pCurrent->getDoNotUnloadSubtreeCount() > 0 ||
            pCurrent->getState() != TileLoadState::Unloaded) {
          this->_traversalStack.push_back(pCurrent);
          return *this;
        }
      }

      // The current tile has no relevant siblings, so see if its parent does.
      pCurrent = pParentOfCurrent;
    } else {
      break;
    }
  }

  return *this;
}

LoadedTileEnumerator::const_iterator
LoadedTileEnumerator::const_iterator::operator++(int) {
  const_iterator copy = *this;
  ++copy;
  return copy;
}

bool LoadedTileEnumerator::const_iterator::operator==(
    const const_iterator& rhs) const noexcept {
  if (this->_traversalStack.size() != rhs._traversalStack.size()) {
    return false;
  } else if (this->_traversalStack.empty() && rhs._traversalStack.empty()) {
    return true;
  } else {
    return this->_traversalStack.back() == rhs._traversalStack.back();
  }
}

bool LoadedTileEnumerator::const_iterator::operator!=(
    const const_iterator& rhs) const noexcept {
  return !(*this == rhs);
}

} // namespace Cesium3DTilesSelection
