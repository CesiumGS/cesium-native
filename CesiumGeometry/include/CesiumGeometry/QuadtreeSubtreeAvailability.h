#pragma once

#include "Library.h"
#include "QuadtreeTileID.h"
#include "QuadtreeTilingScheme.h"
#include "TileAvailabilityFlags.h"

#include <gsl/span>

#include <cstddef>
#include <memory>
#include <vector>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API QuadtreeSubtreeAvailability final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param tilingScheme The {@link QuadtreeTilingScheme} to use with this
   * availability.
   */
  QuadtreeSubtreeAvailability(
      const QuadtreeTilingScheme& tilingScheme,
      uint32_t subtreeLevels,
      uint32_t maximumLevel) noexcept;

  /**
   * @brief Determines the currently known availability status of the given
   * tile.
   *
   * @param tileID The {@link CesiumGeometry::QuadtreeTileID} for the tile.
   *
   * @return The {@link TileAvailabilityFlags} for this tile encoded into a
   * uint8_t.
   */
  uint8_t computeAvailability(const QuadtreeTileID& tileID) const noexcept;

  /**
   * @brief Attempts to add an availability subtree into the existing overall
   * availability tree.
   *
   * @param tileID The {@link CesiumGeometry::QuadtreeTileID} for the tile.
   *
   * @return Whether the insertion was successful.
   */
  bool addSubtree(
      const QuadtreeTileID& tileID,
      std::vector<std::byte>&& data) noexcept;

private:
  struct Subtree {
    std::vector<std::byte> bitstream;
    gsl::span<const std::byte> tileAvailability;
    gsl::span<const std::byte> contentAvailability;
    gsl::span<const std::byte> subtreeAvailability;
  };

  struct Node {
    Subtree subtree;
    std::vector<std::unique_ptr<Node>> childNodes;

    Node(Subtree&& subtree_);
  };

  std::optional<Subtree>
  _createSubtree(std::vector<std::byte>&& data) const noexcept;

  QuadtreeTilingScheme _tilingScheme;
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  std::unique_ptr<Node> _pRoot;

  // precomputed values
  uint32_t _2PowSubtreeLevels;
  size_t _subtreeAvailabilitySize;
  size_t _tileAvailabilitySize;
  size_t _tilePlusContentAvailabilitySize;
  size_t _subtreeBufferSize;
};

} // namespace CesiumGeometry