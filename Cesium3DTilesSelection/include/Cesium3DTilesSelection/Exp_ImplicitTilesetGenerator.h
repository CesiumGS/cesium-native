#pragma once

#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGltf/Model.h>
#include <cstddef>
#include <vector>
#include <filesystem>
#include <unordered_map>

namespace Cesium3DTilesSelection {
class OctreeSubtree {
public:
  OctreeSubtree(uint32_t subtreeLevels);

  void
  markTileContent(uint32_t relativeTileLevel, uint64_t relativeTileMortonId);

  void markTileEmptyContent(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId);

  void markAvailable(
      uint64_t levelOffset,
      uint64_t relativeTileMortonId,
      bool isAvailable,
      std::vector<std::byte>& available);

  bool isTileAvailable(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId) const;

  uint32_t _subtreeLevels;
  std::vector<std::byte> _tileAvailable;
  std::vector<std::byte> _contentAvailable;
  std::vector<std::byte> _subtreeAvailable;
};

class Octree {
public:
  Octree(uint64_t maxLevels, uint32_t subtreeLevels);

  void markTileContent(const CesiumGeometry::OctreeTileID& octreeID);

  void markTileEmptyContent(const CesiumGeometry::OctreeTileID& octreeID);

  const std::vector<std::unordered_map<uint64_t, OctreeSubtree>>&
  getAvailableSubtrees() const noexcept;

  uint64_t getMaxLevels() const noexcept;

  uint32_t getSubtreeLevels() const noexcept;

private:
  CesiumGeometry::OctreeTileID
  calcSubtreeID(const CesiumGeometry::OctreeTileID& octreeID);

  std::vector<std::unordered_map<uint64_t, OctreeSubtree>> _availableSubtrees;
  uint64_t _maxLevels;
  uint32_t _subtreeLevels;
};

struct StaticMesh {
  std::vector<glm::vec3> positions;
  std::vector<uint32_t> indices;
  glm::vec3 min;
  glm::vec3 max;

  CesiumGltf::Model convertToGltf();
};

struct SphereGenerator {
  StaticMesh generate(const glm::vec3& center, float radius);
};

struct ImplicitSerializer {
  void serializeOctree(const Octree& octree, const std::filesystem::path& path);

  void serializeSubtree(
      const OctreeSubtree& subtree,
      const std::filesystem::path& path,
      uint32_t level,
      uint32_t x,
      uint32_t y,
      uint32_t z);

  void serializeGltf(
      const std::filesystem::path &path,
      const OctreeSubtree& subtree,
      const CesiumGeometry::OctreeTileID& subtreeID,
      const CesiumGeometry::OctreeTileID& tileID);
};
} // namespace Cesium3DTilesSelection
