#pragma once

#include <Cesium3DTiles/SubtreeAvailability.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <rapidjson/document.h>

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
 * @brief A representation of the content of a 3D Tiles subtree. Content
 * availability is treated as one-to-one with tile availability.
 */
struct SubtreeContent {
  using Input = std::variant<bool, std::vector<CesiumGeometry::QuadtreeTileID>>;

  SubtreeContent(
      uint32_t maxSubtreeLevel,
      const Input& tileAvailabilities,
      const Input& subtreeAvailabilities);

  /**
   * @brief Outputs the subtree content as a JSON document.
   */
  rapidjson::Document toJson(const std::string& bufferUrl) const;

  /**
   * @brief Outputs the subtree content as a binary subtree file from 3D
   * Tiles 1.1.
   */
  std::vector<std::byte> toBinary() const;

private:
  void initialize(
      uint32_t maxSubtreeLevel,
      const Input& tileAvailabilities,
      const Input& subtreeAvailabilities);

public:
  std::vector<std::byte> buffers;
  Cesium3DTiles::SubtreeAvailability::AvailabilityView tileAvailability;
  Cesium3DTiles::SubtreeAvailability::AvailabilityView subtreeAvailability;
  Cesium3DTiles::SubtreeAvailability::AvailabilityView contentAvailability;
};

/**
 * @brief Computes the total number of tiles under a quadtree with the specified
 * maximum level.
 */
uint64_t getTotalNumberOfTilesForQuadtree(uint64_t maximumLevel);

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
