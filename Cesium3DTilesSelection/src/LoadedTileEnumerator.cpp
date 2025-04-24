#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumUtility/Assert.h>

#include <span>

namespace Cesium3DTilesSelection {

namespace {

bool meetsCriteriaForEnumeration(const Tile* pTile) {
  if (pTile == nullptr)
    return false;

  return pTile->getReferenceCount() > 0 ||
         pTile->getState() != TileLoadState::Unloaded;
}

} // namespace

LoadedConstTileEnumerator::LoadedConstTileEnumerator(
    const Tile* pRootTile) noexcept
    : _pRootTile(pRootTile) {}

LoadedConstTileEnumerator::const_iterator
LoadedConstTileEnumerator::begin() const noexcept {
  if (meetsCriteriaForEnumeration(this->_pRootTile))
    return const_iterator(this->_pRootTile);
  else
    return const_iterator(nullptr);
}

LoadedConstTileEnumerator::const_iterator
LoadedConstTileEnumerator::end() const noexcept {
  return const_iterator(nullptr);
}

LoadedConstTileEnumerator::const_iterator::const_iterator(
    const Tile* pRootTile) noexcept
    : _traversalStack() {
  if (pRootTile) {
    this->_traversalStack.push_back(pRootTile);
  }
}

const Tile&
LoadedConstTileEnumerator::const_iterator::operator*() const noexcept {
  return *this->_traversalStack.back();
}

const Tile*
LoadedConstTileEnumerator::const_iterator::operator->() const noexcept {
  return this->_traversalStack.back();
}

LoadedConstTileEnumerator::const_iterator&
LoadedConstTileEnumerator::const_iterator::operator++() noexcept {
  return LoadedConstTileEnumerator::increment(*this);
}

LoadedConstTileEnumerator::const_iterator
LoadedConstTileEnumerator::const_iterator::operator++(int) noexcept {
  const_iterator copy = *this;
  ++copy;
  return copy;
}

bool LoadedConstTileEnumerator::const_iterator::operator==(
    const const_iterator& rhs) const noexcept {
  if (this->_traversalStack.size() != rhs._traversalStack.size()) {
    return false;
  } else if (this->_traversalStack.empty() && rhs._traversalStack.empty()) {
    return true;
  } else {
    return this->_traversalStack.back() == rhs._traversalStack.back();
  }
}

bool LoadedConstTileEnumerator::const_iterator::operator!=(
    const const_iterator& rhs) const noexcept {
  return !(*this == rhs);
}

LoadedTileEnumerator::LoadedTileEnumerator(Tile* pRootTile) noexcept
    : _pRootTile(pRootTile) {}

LoadedTileEnumerator::const_iterator
LoadedTileEnumerator::begin() const noexcept {
  if (meetsCriteriaForEnumeration(this->_pRootTile))
    return const_iterator(this->_pRootTile);
  else
    return const_iterator(nullptr);
}

LoadedTileEnumerator::const_iterator
LoadedTileEnumerator::end() const noexcept {
  return const_iterator(nullptr);
}

LoadedTileEnumerator::iterator LoadedTileEnumerator::begin() noexcept {
  if (meetsCriteriaForEnumeration(this->_pRootTile))
    return iterator(this->_pRootTile);
  else
    return iterator(nullptr);
}

LoadedTileEnumerator::iterator LoadedTileEnumerator::end() noexcept {
  return iterator(nullptr);
}

LoadedTileEnumerator::iterator::iterator(Tile* pRootTile) noexcept
    : _traversalStack() {
  if (pRootTile) {
    this->_traversalStack.push_back(pRootTile);
  }
}

Tile& LoadedTileEnumerator::iterator::operator*() const noexcept {
  return *this->_traversalStack.back();
}

Tile* LoadedTileEnumerator::iterator::operator->() const noexcept {
  return this->_traversalStack.back();
}

LoadedTileEnumerator::iterator&
LoadedTileEnumerator::iterator::operator++() noexcept {
  return LoadedConstTileEnumerator::increment(*this);
}

LoadedTileEnumerator::iterator
LoadedTileEnumerator::iterator::operator++(int) noexcept {
  iterator copy = *this;
  ++copy;
  return copy;
}

bool LoadedTileEnumerator::iterator::operator==(
    const iterator& rhs) const noexcept {
  if (this->_traversalStack.size() != rhs._traversalStack.size()) {
    return false;
  } else if (this->_traversalStack.empty() && rhs._traversalStack.empty()) {
    return true;
  } else {
    return this->_traversalStack.back() == rhs._traversalStack.back();
  }
}

bool LoadedTileEnumerator::iterator::operator!=(
    const iterator& rhs) const noexcept {
  return !(*this == rhs);
}

template <typename TIterator>
/*static*/ TIterator& LoadedConstTileEnumerator::increment(TIterator& it) {
  if (it._traversalStack.empty())
    return it;

  typename TIterator::pointer pCurrent = it._traversalStack.back();

  // See if we can traverse down into a child tile.
  std::span<typename TIterator::value_type> children = pCurrent->getChildren();
  for (typename TIterator::reference child : children) {
    if (meetsCriteriaForEnumeration(&child)) {
      it._traversalStack.push_back(&child);
      return it;
    }
  }

  // The current tile has no relevant children, so the next
  // tile in the traversal is this tile's sibling, if any.
  while (true) {
    it._traversalStack.pop_back();

    if (!it._traversalStack.empty()) {
      typename TIterator::pointer pParentOfCurrent = it._traversalStack.back();
      std::span<typename TIterator::value_type> childrenOfParent =
          pParentOfCurrent->getChildren();

      CESIUM_ASSERT(!childrenOfParent.empty());
      CESIUM_ASSERT(pCurrent >= &childrenOfParent.front());

      for (++pCurrent; pCurrent <= &childrenOfParent.back(); ++pCurrent) {
        if (meetsCriteriaForEnumeration(pCurrent)) {
          it._traversalStack.push_back(pCurrent);
          return it;
        }
      }

      // The current tile has no relevant siblings, so see if its parent does.
      pCurrent = pParentOfCurrent;
    } else {
      break;
    }
  }

  return it;
}

} // namespace Cesium3DTilesSelection
