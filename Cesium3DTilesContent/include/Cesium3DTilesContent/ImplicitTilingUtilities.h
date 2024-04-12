#pragma once

#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <array>
#include <iterator>
#include <string>

namespace CesiumGeospatial {
class BoundingRegion;
class S2CellBoundingVolume;
} // namespace CesiumGeospatial

namespace CesiumGeometry {
class OrientedBoundingBox;
}

namespace Cesium3DTiles {
struct BoundingVolume;
}

namespace Cesium3DTilesContent {

/**
 * @brief A lightweight virtual container enumerating the quadtree IDs of the
 * children of a given quadtree tile.
 */
class QuadtreeChildren {
public:
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = CesiumGeometry::QuadtreeTileID;
    using difference_type = void;
    using pointer = CesiumGeometry::QuadtreeTileID*;
    using reference = CesiumGeometry::QuadtreeTileID&;

    explicit iterator(
        const CesiumGeometry::QuadtreeTileID& parentTileID,
        bool isFirst) noexcept;

    const CesiumGeometry::QuadtreeTileID& operator*() const {
      return this->_current;
    }
    const CesiumGeometry::QuadtreeTileID* operator->() const {
      return &this->_current;
    }
    iterator& operator++();
    iterator operator++(int);

    bool operator==(const iterator& rhs) const noexcept;
    bool operator!=(const iterator& rhs) const noexcept;

  private:
    CesiumGeometry::QuadtreeTileID _current;
  };

  using const_iterator = iterator;

  QuadtreeChildren(const CesiumGeometry::QuadtreeTileID& tileID) noexcept
      : _tileID(tileID) {}
  iterator begin() const noexcept;
  iterator end() const noexcept;
  constexpr int64_t size() const noexcept { return 4; }

private:
  CesiumGeometry::QuadtreeTileID _tileID;
};

/**
 * @brief A lightweight virtual container enumerating the octree IDs of the
 * children of a given octree tile.
 */
class OctreeChildren {
public:
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = CesiumGeometry::OctreeTileID;
    using difference_type = void;
    using pointer = CesiumGeometry::OctreeTileID*;
    using reference = CesiumGeometry::OctreeTileID&;

    explicit iterator(
        const CesiumGeometry::OctreeTileID& parentTileID,
        bool isFirst) noexcept;

    const CesiumGeometry::OctreeTileID& operator*() const {
      return this->_current;
    }
    const CesiumGeometry::OctreeTileID* operator->() const {
      return &this->_current;
    }
    iterator& operator++();
    iterator operator++(int);

    bool operator==(const iterator& rhs) const noexcept;
    bool operator!=(const iterator& rhs) const noexcept;

  private:
    CesiumGeometry::OctreeTileID _current;
  };

  using const_iterator = iterator;

  OctreeChildren(const CesiumGeometry::OctreeTileID& tileID) noexcept
      : _tileID(tileID) {}
  iterator begin() const noexcept;
  iterator end() const noexcept;
  constexpr int64_t size() const noexcept { return 8; }

private:
  CesiumGeometry::OctreeTileID _tileID;
};

/**
 * @brief Helper functions for working with 3D Tiles implicit tiling.
 */
class ImplicitTilingUtilities {
public:
  /**
   * @brief Resolves a templatized implicit tiling URL with a quadtree tile ID.
   *
   * @param baseUrl The base URL that is used to resolve the urlTemplate if it
   * is a relative path.
   * @param urlTemplate The templatized URL.
   * @param quadtreeID The quadtree ID to use in resolving the parameters in the
   * URL template.
   * @return The resolved URL.
   */
  static std::string resolveUrl(
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const CesiumGeometry::QuadtreeTileID& quadtreeID);

  /**
   * @brief Resolves a templatized implicit tiling URL with an octree tile ID.
   *
   * @param baseUrl The base URL that is used to resolve the urlTemplate if it
   * is a relative path.
   * @param urlTemplate The templatized URL.
   * @param octreeID The octree ID to use in resolving the parameters in the
   * URL template.
   * @return The resolved URL.
   */
  static std::string resolveUrl(
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const CesiumGeometry::OctreeTileID& octreeID);

  /**
   * @brief Computes the denominator for a given implicit tile level.
   *
   * Divide the root tile's geometric error by this value to get the standard
   * geometric error for tiles on the level. Or divide each component of a
   * bounding volume by this factor to get the size of the bounding volume along
   * that axis for tiles of this level.
   *
   * @param level The tile level.
   * @return The denominator for the level.
   */
  static double computeLevelDenominator(uint32_t level) noexcept;

  /**
   * @brief Computes the Morton index for a given quadtree tile within its
   * level.
   *
   * @param tileID The ID of the tile.
   * @return The Morton index.
   */
  static uint64_t
  computeMortonIndex(const CesiumGeometry::QuadtreeTileID& tileID);

  /**
   * @brief Computes the Morton index for a given octree tile within its level.
   *
   * @param tileID The ID of the tile.
   * @return The Morton index.
   */
  static uint64_t
  computeMortonIndex(const CesiumGeometry::OctreeTileID& tileID);

  /**
   * @brief Computes the relative Morton index for a given quadtree tile within
   * its level of a subtree root at the tile with the given quadtree ID.
   *
   * @param subtreeID The ID of the subtree the contains the tile.
   * @param tileID The ID of the tile.
   * @return The relative Morton index.
   */
  static uint64_t computeRelativeMortonIndex(
      const CesiumGeometry::QuadtreeTileID& subtreeID,
      const CesiumGeometry::QuadtreeTileID& tileID);

  /**
   * @brief Computes the relative Morton index for a given octree tile within
   * its level of a subtree rooted at the tile with the given octree ID.
   *
   * @param subtreeID The ID of the subtree the contains the tile.
   * @param tileID The ID of the tile.
   * @return The relative Morton index.
   */
  static uint64_t computeRelativeMortonIndex(
      const CesiumGeometry::OctreeTileID& subtreeRootID,
      const CesiumGeometry::OctreeTileID& tileID);

  /**
   * @brief Gets the ID of the root tile of the subtree that contains a given
   * tile.
   *
   * @param subtreeLevels The number of levels in each sub-tree. For example, if
   * this parameter is 4, then the first subtree starts at level 0 and
   * contains tiles in levels 0 through 3, and the next subtree starts at
   * level 4 and contains tiles in levels 4 through 7.
   * @param tileID The tile ID for each to find the subtree root.
   * @return The ID of the root tile of the subtree.
   */
  static CesiumGeometry::QuadtreeTileID getSubtreeRootID(
      uint32_t subtreeLevels,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Gets the ID of the root tile of the subtree that contains a given
   * tile.
   *
   * @param subtreeLevels The number of levels in each sub-tree. For example, if
   * this parameter is 4, then the first subtree starts at level 0 and
   * contains tiles in levels 0 through 3, and the next subtree starts at
   * level 4 and contains tiles in levels 4 through 7.
   * @param tileID The tile ID for each to find the subtree root.
   * @return The ID of the root tile of the subtree.
   */
  static CesiumGeometry::OctreeTileID getSubtreeRootID(
      uint32_t subtreeLevels,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Converts an absolute tile ID to a tile ID relative to a given root
   * tile.
   *
   * For example, if `rootID` and `tileID` are the same, this method returns
   * `QuadtreeTileID(0, 0, 0)`.
   *
   * @param rootID The ID of the root tile that the returned ID should be
   * relative to.
   * @param tileID The absolute ID of the tile to compute a relative ID for.
   * @return The relative tile ID.
   */
  static CesiumGeometry::QuadtreeTileID absoluteTileIDToRelative(
      const CesiumGeometry::QuadtreeTileID& rootID,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Converts an absolute tile ID to a tile ID relative to a given root
   * tile.
   *
   * For example, if `rootID` and `tileID` are the same, this method returns
   * `OctreeTileID(0, 0, 0, 0)`.
   *
   * @param rootID The ID of the root tile that the returned ID should be
   * relative to.
   * @param tileID The absolute ID of the tile to compute a relative ID for.
   * @return The relative tile ID.
   */
  static CesiumGeometry::OctreeTileID absoluteTileIDToRelative(
      const CesiumGeometry::OctreeTileID& rootID,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Gets a lightweight virtual container for enumerating the quadtree
   * IDs of the children of a given quadtree tile.
   *
   * @param tileID The tile ID of the parent tile for which to get children.
   * @return The children.
   */
  static QuadtreeChildren
  getChildren(const CesiumGeometry::QuadtreeTileID& tileID) noexcept {
    return QuadtreeChildren{tileID};
  }

  /**
   * @brief Gets a lightweight virtual container for enumerating the octree
   * IDs of the children of a given octree tile.
   *
   * @param tileID The tile ID of the parent tile for which to get children.
   * @return The children.
   */
  static OctreeChildren
  getChildren(const CesiumGeometry::OctreeTileID& tileID) noexcept {
    return OctreeChildren{tileID};
  }

  /**
   * @brief Computes the bounding volume for an implicit quadtree tile with the
   * given ID as a {@link Cesium3DTiles::BoundingVolume}.
   *
   * @param rootBoundingVolume The bounding volume of the root tile.
   * @param tileID The tile ID for which to compute the bounding volume.
   * @return The bounding volume for the given implicit tile.
   */
  static Cesium3DTiles::BoundingVolume computeBoundingVolume(
      const Cesium3DTiles::BoundingVolume& rootBoundingVolume,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit octree tile with the
   * given ID as a {@link Cesium3DTiles::BoundingVolume}.
   *
   * @param rootBoundingVolume The bounding volume of the root tile.
   * @param tileID The tile ID for which to compute the bounding volume.
   * @return The bounding volume for the given implicit tile.
   */
  static Cesium3DTiles::BoundingVolume computeBoundingVolume(
      const Cesium3DTiles::BoundingVolume& rootBoundingVolume,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit quadtree tile with the
   * given ID as a bounding region.
   *
   * @param rootBoundingVolume The bounding region of the root tile.
   * @param tileID The tile ID for which to compute the bounding region.
   * @return The bounding region for the given implicit tile.
   */
  static CesiumGeospatial::BoundingRegion computeBoundingVolume(
      const CesiumGeospatial::BoundingRegion& rootBoundingVolume,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit octree tile with the
   * given ID as a bounding region.
   *
   * @param rootBoundingVolume The bounding region of the root tile.
   * @param tileID The tile ID for which to compute the bounding region.
   * @return The bounding region for the given implicit tile.
   */
  static CesiumGeospatial::BoundingRegion computeBoundingVolume(
      const CesiumGeospatial::BoundingRegion& rootBoundingVolume,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit quadtree tile
   * with the given ID as an oriented bounding box.
   *
   * @param rootBoundingVolume The oriented bounding box of the root tile.
   * @param tileID The tile ID for which to compute the oriented bounding box.
   * @return The oriented bounding box for the given implicit tile.
   */
  static CesiumGeometry::OrientedBoundingBox computeBoundingVolume(
      const CesiumGeometry::OrientedBoundingBox& rootBoundingVolume,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit octree tile with
   * the given ID as an oriented bounding box.
   *
   * @param rootBoundingVolume The oriented bounding box of the root tile.
   * @param tileID The tile ID for which to compute the oriented bounding box.
   * @return The oriented bounding box for the given implicit tile.
   */
  static CesiumGeometry::OrientedBoundingBox computeBoundingVolume(
      const CesiumGeometry::OrientedBoundingBox& rootBoundingVolume,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit quadtree tile
   * with the given ID as an S2 cell bounding volume.
   *
   * @param rootBoundingVolume The S2 cell bounding volume of the root tile.
   * @param tileID The tile ID for which to compute the S2 cell bounding volume.
   * @return The S2 cell bounding volume for the given implicit tile.
   */
  static CesiumGeospatial::S2CellBoundingVolume computeBoundingVolume(
      const CesiumGeospatial::S2CellBoundingVolume& rootBoundingVolume,
      const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  /**
   * @brief Computes the bounding volume for an implicit octree tile
   * with the given ID as an S2 cell bounding volume.
   *
   * @param rootBoundingVolume The S2 cell bounding volume of the root tile.
   * @param tileID The tile ID for which to compute the S2 cell bounding volume.
   * @return The S2 cell bounding volume for the given implicit tile.
   */
  static CesiumGeospatial::S2CellBoundingVolume computeBoundingVolume(
      const CesiumGeospatial::S2CellBoundingVolume& rootBoundingVolume,
      const CesiumGeometry::OctreeTileID& tileID) noexcept;
};

} // namespace Cesium3DTilesContent
