#pragma once

#include <iterator>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;

class LoadedTileEnumerator {
public:
  class const_iterator {
  public:
    /**
     * @brief The iterator category tag denoting this is a forward iterator.
     */
    using iterator_category = std::forward_iterator_tag;
    /**
     * @brief The type of value that is being iterated over.
     */
    using value_type = Tile;
    /**
     * @brief The type used to identify distance between iterators.
     *
     * This is `void` as the distance between two tiles isn't
     * particularly useful.
     */
    using difference_type = void;
    /**
     * @brief A pointer to the type being iterated over.
     */
    using pointer = Tile*;
    /**
     * @brief A reference to the type being iterated over.
     */
    using reference = Tile&;

    explicit const_iterator(const Tile* pRootTile);

    /**
     * @brief Returns a reference to the current \ref
     * CesiumGeometry::QuadtreeTileID being iterated.
     */
    const Tile& operator*() const { return *this->_traversalStack.back(); }
    /**
     * @brief Returns a pointer to the current \ref
     * CesiumGeometry::QuadtreeTileID being iterated.
     */
    const Tile* operator->() const { return this->_traversalStack.back(); }
    /**
     * @brief Advances the iterator to the next child.
     */
    const_iterator& operator++();
    /**
     * @brief Advances the iterator to the next child.
     */
    const_iterator operator++(int);

    /** @brief Checks if two iterators are at the same child. */
    bool operator==(const const_iterator& rhs) const noexcept;
    /** @brief Checks if two iterators are NOT at the same child. */
    bool operator!=(const const_iterator& rhs) const noexcept;

  private:
    std::vector<const Tile*> _traversalStack;
  };

  void updateRootTile(const Tile* pRootTile);

  const_iterator begin() const;
  const_iterator end() const;

private:
  const Tile* _pRootTile = nullptr;
};

} // namespace Cesium3DTilesSelection
