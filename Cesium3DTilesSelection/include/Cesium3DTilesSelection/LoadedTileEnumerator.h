#pragma once

#include <iterator>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class TilesetContentManager;

// The LoadedConstTileEnumerator and LoadedTileEnumerator could probably be
// replaced with ranges, except that we need to support Android and range
// support prior to NDK r26 is shaky. Unreal Engine 5.5 uses r25b and Unity
// 2022.3 uses r23b. See here: https://github.com/android/ndk/issues/1530

class LoadedConstTileEnumerator {
public:
  class const_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Tile;
    using difference_type = void;
    using pointer = const Tile*;
    using reference = const Tile&;

    explicit const_iterator(const Tile* pRootTile) noexcept;

    const Tile& operator*() const noexcept;
    const Tile* operator->() const noexcept;

    const_iterator& operator++() noexcept;
    const_iterator operator++(int) noexcept;

    bool operator==(const const_iterator& rhs) const noexcept;
    bool operator!=(const const_iterator& rhs) const noexcept;

  private:
    std::vector<const Tile*> _traversalStack;
    friend class LoadedConstTileEnumerator;
  };

  explicit LoadedConstTileEnumerator(const Tile* pRootTile) noexcept;

  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;

private:
  const Tile* _pRootTile;

  template <typename TIterator> static TIterator& increment(TIterator& it);
  friend class LoadedTileEnumerator;
};

class LoadedTileEnumerator {
public:
  using const_iterator = LoadedConstTileEnumerator::const_iterator;

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Tile;
    using difference_type = void;
    using pointer = Tile*;
    using reference = Tile&;

    explicit iterator(Tile* pRootTile) noexcept;

    Tile& operator*() const noexcept;
    Tile* operator->() const noexcept;

    iterator& operator++() noexcept;
    iterator operator++(int) noexcept;

    bool operator==(const iterator& rhs) const noexcept;
    bool operator!=(const iterator& rhs) const noexcept;

  private:
    std::vector<Tile*> _traversalStack;
    friend class LoadedConstTileEnumerator;
  };

  explicit LoadedTileEnumerator(Tile* pRootTile) noexcept;

  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;

  iterator begin() noexcept;
  iterator end() noexcept;

private:
  Tile* _pRootTile;
};

} // namespace Cesium3DTilesSelection
