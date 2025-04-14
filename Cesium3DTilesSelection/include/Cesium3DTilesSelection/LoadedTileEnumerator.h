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

/**
 * @brief A "virtual collection" that allows enumeration through the loaded
 * tiles in a subtree rooted at a given {@link Tile}.
 *
 * For the purposes of this enumeration, a loaded tile is one that is in a
 * {@link TileLoadState} other than {@link TileLoadState::Unloaded}, or that
 * has any children (or other descendants) that meet this criteria. We check the
 * latter criteria by looking at {@link Tile::getReferenceCount}.
 */
class LoadedConstTileEnumerator {
public:
  /**
   * @brief An iterator over constant {@link Tile} instances.
   */
  class const_iterator {
  public:
    /**
     * @brief The iterator category tag denoting this is a forward iterator.
     */
    using iterator_category = std::forward_iterator_tag;
    /**
     * @brief The type of value that is being iterated over.
     */
    using value_type = const Tile;
    /**
     * @brief The type used to identify distance between iterators.
     *
     * This is `void` because there is no meaningful measure of distance between
     * tiles.
     */
    using difference_type = void;
    /**
     * @brief A pointer to the type being iterated over.
     */
    using pointer = const Tile*;
    /**
     * @brief A reference to the type being iterated over.
     */
    using reference = const Tile&;

    /**
     * @brief Returns a reference to the current item being iterated.
     */
    const Tile& operator*() const noexcept;
    /**
     * @brief Returns a pointer to the current item being iterated.
     */
    const Tile* operator->() const noexcept;

    /**
     * @brief Advances the iterator to the next item (pre-incrementing).
     */
    const_iterator& operator++() noexcept;
    /**
     * @brief Advances the iterator to the next item (post-incrementing).
     */
    const_iterator operator++(int) noexcept;

    /** @brief Checks if two iterators are at the same item. */
    bool operator==(const const_iterator& rhs) const noexcept;
    /** @brief Checks if two iterators are not at the same item. */
    bool operator!=(const const_iterator& rhs) const noexcept;

  private:
    explicit const_iterator(const Tile* pRootTile) noexcept;

    std::vector<const Tile*> _traversalStack;

    friend class LoadedTileEnumerator;
    friend class LoadedConstTileEnumerator;
  };

  /**
   * @brief Creates a new instance to enumerate loaded tiles in the subtree
   * rooted at `pRootTile`.
   *
   * If `pRootTile` is `nullptr`, then the iteration is empty (`begin==end`).
   * Otherwise, the iteration will include at least `pRootTile`, even if it is
   * not loaded.
   *
   * @param pRootTile The root tile of the subtree.
   */
  explicit LoadedConstTileEnumerator(const Tile* pRootTile) noexcept;

  /** @brief Returns an iterator starting at the first tile. */
  const_iterator begin() const noexcept;
  /** @brief Returns an iterator starting after the last tile. */
  const_iterator end() const noexcept;

private:
  const Tile* _pRootTile;

  template <typename TIterator> static TIterator& increment(TIterator& it);
  friend class LoadedTileEnumerator;
};

/** @copydoc LoadedConstTileEnumerator */
class LoadedTileEnumerator {
public:
  /**
   * @brief An iterator over constant {@link Tile} instances.
   */
  using const_iterator = LoadedConstTileEnumerator::const_iterator;

  /**
   * @brief An iterator over {@link Tile} instances.
   */
  class iterator {
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
     * This is `void` because there is no meaningful measure of distance between
     * tiles.
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

    /**
     * @brief Returns a reference to the current item being iterated.
     */
    Tile& operator*() const noexcept;
    /**
     * @brief Returns a pointer to the current item being iterated.
     */
    Tile* operator->() const noexcept;

    /**
     * @brief Advances the iterator to the next item (pre-incrementing).
     */
    iterator& operator++() noexcept;
    /**
     * @brief Advances the iterator to the next item (post-incrementing).
     */
    iterator operator++(int) noexcept;

    /** @brief Checks if two iterators are at the same item. */
    bool operator==(const iterator& rhs) const noexcept;
    /** @brief Checks if two iterators are not at the same item. */
    bool operator!=(const iterator& rhs) const noexcept;

  private:
    explicit iterator(Tile* pRootTile) noexcept;

    std::vector<Tile*> _traversalStack;

    friend class LoadedTileEnumerator;
    friend class LoadedConstTileEnumerator;
  };

  /** @copydoc LoadedConstTileEnumerator::LoadedConstTileEnumerator */
  explicit LoadedTileEnumerator(Tile* pRootTile) noexcept;

  /** @brief Returns an iterator starting at the first tile. */
  const_iterator begin() const noexcept;
  /** @brief Returns an iterator starting after the last tile. */
  const_iterator end() const noexcept;

  /** @brief Returns an iterator starting at the first tile. */
  iterator begin() noexcept;
  /** @brief Returns an iterator starting after the last tile. */
  iterator end() noexcept;

private:
  Tile* _pRootTile;
};

} // namespace Cesium3DTilesSelection
