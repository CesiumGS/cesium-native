#pragma once

#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <array>
#include <string>

namespace CesiumGeometry {
struct QuadtreeTileID;
struct OctreeTileID;
} // namespace CesiumGeometry

namespace Cesium3DTilesContent {

class QuadtreeChildIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = CesiumGeometry::QuadtreeTileID;
  using pointer = CesiumGeometry::QuadtreeTileID*;
  using reference = CesiumGeometry::QuadtreeTileID&;

  explicit QuadtreeChildIterator(
      const CesiumGeometry::QuadtreeTileID& parentTileID,
      bool isFirst) noexcept;

  const CesiumGeometry::QuadtreeTileID& operator*() const {
    return this->_current;
  }
  const CesiumGeometry::QuadtreeTileID* operator->() const {
    return &this->_current;
  }
  QuadtreeChildIterator& operator++();
  QuadtreeChildIterator operator++(int);

  bool operator==(const QuadtreeChildIterator& rhs) const noexcept;
  bool operator!=(const QuadtreeChildIterator& rhs) const noexcept;

private:
  CesiumGeometry::QuadtreeTileID _current;
};

class OctreeChildIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = CesiumGeometry::OctreeTileID;
  using pointer = CesiumGeometry::OctreeTileID*;
  using reference = CesiumGeometry::OctreeTileID&;

  explicit OctreeChildIterator(
      const CesiumGeometry::OctreeTileID& parentTileID,
      bool isFirst) noexcept;

  const CesiumGeometry::OctreeTileID& operator*() const {
    return this->_current;
  }
  const CesiumGeometry::OctreeTileID* operator->() const {
    return &this->_current;
  }
  OctreeChildIterator& operator++();
  OctreeChildIterator operator++(int);

  bool operator==(const OctreeChildIterator& rhs) const noexcept;
  bool operator!=(const OctreeChildIterator& rhs) const noexcept;

private:
  CesiumGeometry::OctreeTileID _current;
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
   * @brief Computes the relative Morton index for a given quadtree tile within
   * a subtree with the given quadtree ID.
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
   * a subtree with the given octree ID.
   *
   * @param subtreeID The ID of the subtree the contains the tile.
   * @param tileID The ID of the tile.
   * @return The relative Morton index.
   */
  static uint64_t computeRelativeMortonIndex(
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID);

  static QuadtreeChildIterator
  childrenBegin(const CesiumGeometry::QuadtreeTileID& tileID) noexcept;
  static QuadtreeChildIterator
  childrenEnd(const CesiumGeometry::QuadtreeTileID& tileID) noexcept;

  static OctreeChildIterator
  childrenBegin(const CesiumGeometry::OctreeTileID& tileID) noexcept;
  static OctreeChildIterator
  childrenEnd(const CesiumGeometry::OctreeTileID& tileID) noexcept;

  /**
   * @brief Gets the quadtree tile IDs of the four children of a given quadtree
   * tile.
   *
   * @param parentTileID The ID of the parent tile.
   * @return The IDs of the four children.
   */
  static std::array<CesiumGeometry::QuadtreeTileID, 4>
  getChildTileIDs(const CesiumGeometry::QuadtreeTileID& parentTileID);

  /**
   * @brief Gets the octree tile IDs of the eight children of a given octree
   * tile.
   *
   * @param parentTileID The ID of the parent tile.
   * @return The IDs of the four children.
   */
  static std::array<CesiumGeometry::OctreeTileID, 8>
  getChildTileIDs(const CesiumGeometry::OctreeTileID& parentTileID);
};

} // namespace Cesium3DTilesContent
