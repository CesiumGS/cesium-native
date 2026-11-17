#pragma once

#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <rapidjson/Document.h>

#include <cstddef>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace CesiumNativeTests {
/**
 * @brief The header of a binary subtree file in 3D Tiles 1.1.
 */
struct SubtreeHeader {
  char magic[4];
  uint32_t version;
  uint64_t jsonByteLength;
  uint64_t binaryByteLength;
};

/**
 * @brief A representation of the content of a 3D Tiles subtree.
 */
struct SubtreeContent {
  using Input = std::variant<bool, std::vector<CesiumGeometry::QuadtreeTileID>>;

  std::vector<std::byte> buffers;
  Cesium3DTilesContent::SubtreeAvailability::AvailabilityView tileAvailability;
  Cesium3DTilesContent::SubtreeAvailability::AvailabilityView
      subtreeAvailability;
  Cesium3DTilesContent::SubtreeAvailability::AvailabilityView
      contentAvailability;

  SubtreeContent(
      uint32_t maxSubtreeLevels,
      const Input& tileAvailabilities,
      const Input& subtreeAvailabilities);

  rapidjson::Document toJson(const std::string& bufferUrl);
};

/**
 * @brief Computes the total number of tiles under a quadtree with the specified
 * level count.
 */
uint64_t getTotalNumberOfTilesForQuadtree(uint64_t subtreeLevels);

/**
 * @brief Marks the tile at the specified ID as available in the specified
 * availability bitstream.
 *
 * @param tileID The tile ID.
 * @param available The availability bitstream.
 */
void markTileAvailable(
    const CesiumGeometry::QuadtreeTileID& tileID,
    std::span<std::byte> available);

/**
 * @brief Marks the subtree at the specified ID as available in the specified
 * availability bitstream.
 *
 * @param tileID The tile ID.
 * @param available The availability bitstream.
 */
void markSubtreeAvailable(
    const CesiumGeometry::QuadtreeTileID& tileID,
    std::span<std::byte> available);

} // namespace CesiumNativeTests
