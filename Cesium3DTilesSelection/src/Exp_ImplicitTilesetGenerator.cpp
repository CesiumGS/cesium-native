#include <Cesium3DTilesSelection/Exp_ImplicitTilesetGenerator.h>
#include <Cesium3DTilesWriter/SubtreeWriter.h>
#include <Cesium3DTilesWriter/TilesetWriter.h>
#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTiles/Tileset.h>
#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTiles/Extension3dTilesImplicitTiling.h>
#include <spdlog/fmt/fmt.h>
#include <libmorton/morton.h>
#include <fstream>

namespace Cesium3DTilesSelection {
OctreeSubtree::OctreeSubtree(uint32_t subtreeLevels) {
  uint64_t numOfTiles = ((1 << (3 * (subtreeLevels + 1))) - 1) / (8 - 1);

  _tileAvailable.resize(uint64_t(std::ceil(numOfTiles / 8)));
  _contentAvailable.resize(uint64_t(std::ceil(numOfTiles / 8)));
  _subtreeAvailable.resize(uint64_t(1) << (3 * (subtreeLevels)));
}

void OctreeSubtree::markTileContent(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId) {
  uint64_t numOfTilesFromRootToParentLevel =
      ((1UL << (3UL * relativeTileLevel)) - 1) / (8 - 1);
  markAvailable(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      true,
      _tileAvailable);
  markAvailable(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      true,
      _contentAvailable);
}

void OctreeSubtree::markTileEmptyContent(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId) {
  uint64_t numOfTilesFromRootToParentLevel =
      ((1 << (3 * relativeTileLevel)) - 1) / (8 - 1);
  markAvailable(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      true,
      _tileAvailable);
  markAvailable(
      numOfTilesFromRootToParentLevel,
      relativeTileMortonId,
      false,
      _contentAvailable);
}

void OctreeSubtree::markAvailable(
    uint64_t levelOffset,
    uint64_t relativeTileMortonId,
    bool isAvailable,
    std::vector<std::byte>& available) {
  uint64_t availabilityBitIndex = levelOffset + relativeTileMortonId;
  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  if (isAvailable) {
    available[byteIndex] |= std::byte(1 << bitIndex);
  } else {
    available[byteIndex] &= std::byte(~(1 << bitIndex));
  }
}

Octree::Octree(uint64_t maxLevels, uint32_t subtreeLevels)
    : _maxLevels{maxLevels}, _subtreeLevels{subtreeLevels}, _availableSubtrees(maxLevels / subtreeLevels) {}

void Octree::markTileContent(const CesiumGeometry::OctreeTileID& octreeID) {
  CesiumGeometry::OctreeTileID subtreeID = calcSubtreeID(octreeID);
  uint64_t subtreeLevelIdx = subtreeID.level / _subtreeLevels;
  uint64_t subtreeMortonID =
      libmorton::morton3D_64_encode(subtreeID.x, subtreeID.y, subtreeID.z);
  auto subtreeIt = _availableSubtrees[subtreeLevelIdx].find(subtreeMortonID);
  if (subtreeIt == _availableSubtrees[subtreeLevelIdx].end()) {
    auto result = _availableSubtrees[subtreeLevelIdx].insert(
        {subtreeMortonID, OctreeSubtree(_subtreeLevels)});
    subtreeIt = result.first;
  }

  uint32_t relativeTileLevel = octreeID.level - subtreeID.level;
  uint64_t relativeTileMortonIdx = libmorton::morton3D_64_encode(
      octreeID.x - (subtreeID.x << relativeTileLevel),
      octreeID.y - (subtreeID.y << relativeTileLevel),
      octreeID.z - (subtreeID.z << relativeTileLevel));

  subtreeIt->second.markTileContent(relativeTileLevel, relativeTileMortonIdx);
}

void Octree::markTileEmptyContent(
    const CesiumGeometry::OctreeTileID& octreeID) {
  CesiumGeometry::OctreeTileID subtreeID = calcSubtreeID(octreeID);
  uint64_t subtreeLevelIdx = subtreeID.level / _subtreeLevels;
  uint64_t subtreeMortonID =
      libmorton::morton3D_64_encode(subtreeID.x, subtreeID.y, subtreeID.z);
  auto subtreeIt = _availableSubtrees[subtreeLevelIdx].find(subtreeMortonID);
  if (subtreeIt == _availableSubtrees[subtreeLevelIdx].end()) {
    auto result = _availableSubtrees[subtreeLevelIdx].insert(
        {subtreeMortonID, OctreeSubtree(_subtreeLevels)});
    subtreeIt = result.first;
  }

  uint32_t relativeTileLevel = octreeID.level - subtreeID.level;
  uint64_t relativeTileMortonIdx = libmorton::morton3D_64_encode(
      octreeID.x - (subtreeID.x << relativeTileLevel),
      octreeID.y - (subtreeID.y << relativeTileLevel),
      octreeID.z - (subtreeID.z << relativeTileLevel));

  subtreeIt->second.markTileEmptyContent(
      relativeTileLevel,
      relativeTileMortonIdx);
}

const std::vector<std::unordered_map<uint64_t, OctreeSubtree>>&
Octree::getAvailableSubtrees() const noexcept {
  return _availableSubtrees;
}

uint64_t Octree::getMaxLevels() const noexcept { return _maxLevels; }

uint32_t Octree::getSubtreeLevels() const noexcept { return _subtreeLevels; }

CesiumGeometry::OctreeTileID
Octree::calcSubtreeID(const CesiumGeometry::OctreeTileID& octreeID) {
  uint32_t subtreeLevelIdx = octreeID.level / _subtreeLevels;
  uint64_t levelLeft = octreeID.level % _subtreeLevels;
  uint32_t subtreeLevel = _subtreeLevels * subtreeLevelIdx;
  uint32_t subtreeX = octreeID.x >> levelLeft;
  uint32_t subtreeY = octreeID.y >> levelLeft;
  uint32_t subtreeZ = octreeID.z >> levelLeft;

  return {subtreeLevel, subtreeX, subtreeY, subtreeZ};
}

CesiumGltf::Model StaticMesh::convertToGltf() {
  CesiumGltf::Model model;

  CesiumGltf::Material& material = model.materials.emplace_back();
  material.pbrMetallicRoughness = CesiumGltf::MaterialPBRMetallicRoughness{};

  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = int64_t(positions.size() * sizeof(glm::vec3));
  buffer.cesium.data.resize(buffer.byteLength);
  std::memcpy(buffer.cesium.data.data(), positions.data(), buffer.byteLength);

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteOffset = 0;
  bufferView.byteStride = 0;
  bufferView.byteLength = buffer.byteLength;

  CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.byteOffset = 0;
  accessor.count = positions.size();
  accessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  accessor.type = CesiumGltf::Accessor::Type::VEC3;

  CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.attributes["POSITION"] = 0;
  primitive.material = 0;

  return model;
}

void ImplicitSerializer::serializeOctree(
    const Octree& octree,
    const std::filesystem::path& path) {
  const auto& availableSubtrees = octree.getAvailableSubtrees();
  for (size_t i = 0; i < availableSubtrees.size(); ++i) {
    uint32_t subtreeLevel =
        static_cast<uint32_t>(i * octree.getSubtreeLevels());
    for (const auto& subtree : availableSubtrees[i]) {
      uint32_t x;
      uint32_t y;
      uint32_t z;
      libmorton::morton3D_64_decode(subtree.first, x, y, z);
      serializeSubtree(subtree.second, path / "subtree", subtreeLevel, x, y, z);
    }
  }

  Cesium3DTiles::Tileset tileset;
  tileset.geometricError = 50.0;

  tileset.root.boundingVolume.box = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    2.0, 2.0, 2.0};
  tileset.root.geometricError = 50.0;
  auto &implicit = tileset.root.addExtension<Cesium3DTiles::Extension3dTilesImplicitTiling>();
  implicit.subdivisionScheme =
      Cesium3DTiles::Extension3dTilesImplicitTiling::SubdivisionScheme::OCTREE;
  implicit.subtreeLevels = octree.getSubtreeLevels();
  implicit.availableLevels = octree.getMaxLevels();
  implicit.subtrees.uri = "subtree/{level}/{x}/{y}/{z}.json";

  Cesium3DTilesWriter::TilesetWriter writer;
  auto result = writer.writeTileset(tileset, {true});
  auto tilesetFile = std::fstream(path / "tileset.json", std::ios::out | std::ios::binary);
  tilesetFile.write((char*)&result.tilesetBytes[0], result.tilesetBytes.size());
}

void ImplicitSerializer::serializeSubtree(
    const OctreeSubtree& octreeSubtree,
    const std::filesystem::path& path,
    uint32_t level,
    uint32_t x,
    uint32_t y,
    uint32_t z) {
  std::vector<std::byte> availableBuffer(
      octreeSubtree._tileAvailable.size() +
      octreeSubtree._contentAvailable.size() +
      octreeSubtree._subtreeAvailable.size());
  std::memcpy(
      availableBuffer.data(),
      octreeSubtree._tileAvailable.data(),
      octreeSubtree._tileAvailable.size());
  std::memcpy(
      availableBuffer.data() + octreeSubtree._tileAvailable.size(),
      octreeSubtree._contentAvailable.data(),
      octreeSubtree._contentAvailable.size());
  std::memcpy(
      availableBuffer.data() + octreeSubtree._tileAvailable.size() +
          octreeSubtree._contentAvailable.size(),
      octreeSubtree._subtreeAvailable.data(),
      octreeSubtree._subtreeAvailable.size());

  auto subtreeDirectory = std::filesystem::path(
      fmt::format((path / "{}" / "{}" / "{}").string(), level, x, y));
  std::filesystem::create_directories(subtreeDirectory);
  auto subtreeFilePath = subtreeDirectory / fmt::format("{}.json", z); 
  auto bufferPath = subtreeDirectory / fmt::format("{}.bin", z);

  Cesium3DTiles::Subtree subtree;

  Cesium3DTiles::Buffer& buffer = subtree.buffers.emplace_back();
  buffer.uri = fmt::format(path.stem().string() + "/{}/{}/{}/{}.bin", level, x, y, z);
  buffer.byteLength = availableBuffer.size();

  Cesium3DTiles::BufferView& tileAvailabilityView =
      subtree.bufferViews.emplace_back();
  tileAvailabilityView.buffer = 0;
  tileAvailabilityView.byteOffset = 0;
  tileAvailabilityView.byteLength = octreeSubtree._tileAvailable.size();

  Cesium3DTiles::BufferView& contentAvailabilityView =
      subtree.bufferViews.emplace_back();
  contentAvailabilityView.buffer = 0;
  contentAvailabilityView.byteOffset = octreeSubtree._tileAvailable.size();
  contentAvailabilityView.byteLength = octreeSubtree._contentAvailable.size();

  Cesium3DTiles::BufferView& subtreeAvailabilityView =
      subtree.bufferViews.emplace_back();
  subtreeAvailabilityView.buffer = 0;
  subtreeAvailabilityView.byteOffset = octreeSubtree._tileAvailable.size() + octreeSubtree._contentAvailable.size();
  subtreeAvailabilityView.byteLength = octreeSubtree._subtreeAvailable.size();

  subtree.tileAvailability.bitstream = 0;
  subtree.contentAvailability.emplace_back().bitstream = 1;
  subtree.childSubtreeAvailability.bitstream = 2;

  Cesium3DTilesWriter::SubtreeWriter writer;
  auto result = writer.writeSubtree(subtree, {true});

  // write buffer
  auto bufferFile = std::fstream(bufferPath, std::ios::out | std::ios::binary);
  bufferFile.write((char*)&availableBuffer[0], availableBuffer.size());

  // write subtree
  auto subtreeFile = std::fstream(subtreeFilePath, std::ios::out | std::ios::binary);
  subtreeFile.write((char*)&result.subtreeBytes[0], result.subtreeBytes.size());
}
} // namespace Cesium3DTilesSelection
