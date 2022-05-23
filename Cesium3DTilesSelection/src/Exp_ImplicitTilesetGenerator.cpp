#include <Cesium3DTilesSelection/Exp_ImplicitTilesetGenerator.h>
#include <Cesium3DTilesWriter/SubtreeWriter.h>
#include <Cesium3DTilesWriter/TilesetWriter.h>
#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTiles/Tileset.h>
#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTiles/Extension3dTilesImplicitTiling.h>
#include <CesiumGltfWriter/GltfWriter.h>
#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>
#include <libmorton/morton.h>
#include <fstream>
#include <cmath>

namespace Cesium3DTilesSelection {
OctreeSubtree::OctreeSubtree(uint32_t subtreeLevels)
    : _subtreeLevels{subtreeLevels} {
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

bool OctreeSubtree::isTileAvailable(
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonId) const {
  uint64_t numOfTilesFromRootToParentLevel =
      ((1 << (3 * relativeTileLevel)) - 1) / (8 - 1);

  uint64_t availabilityBitIndex =
      numOfTilesFromRootToParentLevel + relativeTileMortonId;

  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  const int bitValue =
      static_cast<int>(_tileAvailable[byteIndex] >> bitIndex) & 1;

  return bitValue == 1;
}

Octree::Octree(uint64_t maxLevels, uint32_t subtreeLevels)
    : _maxLevels{maxLevels},
      _subtreeLevels{subtreeLevels},
      _availableSubtrees(maxLevels / subtreeLevels) {}

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
  model.asset.version = "2.0";

  CesiumGltf::Material& material = model.materials.emplace_back();
  material.pbrMetallicRoughness = CesiumGltf::MaterialPBRMetallicRoughness{};

  // create position accessor
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = int64_t(positions.size() * sizeof(glm::vec3)) +
                      int64_t(indices.size()) * sizeof(uint32_t);
  buffer.cesium.data.resize(buffer.byteLength);
  std::memcpy(
      buffer.cesium.data.data(),
      positions.data(),
      positions.size() * sizeof(glm::vec3));
  std::memcpy(
      buffer.cesium.data.data() + positions.size() * sizeof(glm::vec3),
      indices.data(),
      indices.size() * sizeof(uint32_t));

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteOffset = 0;
  bufferView.byteStride = sizeof(glm::vec3);
  bufferView.byteLength = buffer.byteLength;
  bufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.byteOffset = 0;
  accessor.count = positions.size();
  accessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  accessor.type = CesiumGltf::Accessor::Type::VEC3;
  accessor.min = {min.x, min.y, min.z};
  accessor.max = {max.x, max.y, max.z};

  // create indices accessor
  CesiumGltf::BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = 0;
  indicesBufferView.byteOffset = positions.size() * sizeof(glm::vec3);
  indicesBufferView.byteLength = indices.size() * sizeof(uint32_t);
  indicesBufferView.target =
      CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER;

  CesiumGltf::Accessor& indicesAccessor = model.accessors.emplace_back();
  indicesAccessor.bufferView = 1;
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = indices.size();
  indicesAccessor.componentType =
      CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
  indicesAccessor.type = CesiumGltf::Accessor::Type::SCALAR;

  CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.attributes["POSITION"] = 0;
  primitive.indices = 1;
  primitive.material = 0;

  auto& node = model.nodes.emplace_back();
  node.mesh = 0;

  auto& scene = model.scenes.emplace_back();
  scene.nodes.emplace_back(0);

  model.scene = 0;

  return model;
}

StaticMesh SphereGenerator::generate(const glm::vec3& center, float radius) {
  float theta = 0.0;
  float beta = 0.0;
  int totalSegments = 10;
  float ySegmentLength = 180.0f / totalSegments;
  float segmentLength = 360.0f / totalSegments;

  glm::vec3 min{std::numeric_limits<float>::max()};
  glm::vec3 max{std::numeric_limits<float>::min()};
  std::vector<glm::vec3> positions;
  positions.reserve((totalSegments + 1) * (totalSegments + 1));
  for (int y = 0; y < totalSegments + 1; ++y) {
    for (int x = 0; x < totalSegments + 1; ++x) {
      float posZ = center.x + radius * std::sin(glm::radians(theta)) *
                                  std::cos(glm::radians(beta));
      float posX = center.y + radius * std::sin(glm::radians(theta)) *
                                  std::sin(glm::radians(beta));
      float posY = center.z + radius * std::cos(glm::radians(theta));
      positions.emplace_back(posX, posY, posZ);

      min.x = glm::min(posX, min.x);
      min.y = glm::min(posY, min.y);
      min.z = glm::min(posZ, min.z);

      max.x = glm::max(posX, max.x);
      max.y = glm::max(posY, max.y);
      max.z = glm::max(posZ, max.z);

      beta += segmentLength;
    }

    theta += ySegmentLength;
  }

  std::vector<uint32_t> indices;
  indices.reserve((totalSegments) * (totalSegments)*6);
  for (int y = 0; y < totalSegments; ++y) {
    for (int x = 0; x < totalSegments; ++x) {
      indices.emplace_back(y * (totalSegments + 1) + x + 1);
      indices.emplace_back(y * (totalSegments + 1) + x);
      indices.emplace_back((y + 1) * (totalSegments + 1) + x);

      indices.emplace_back(y * (totalSegments + 1) + x + 1);
      indices.emplace_back((y + 1) * (totalSegments + 1) + x);
      indices.emplace_back((y + 1) * (totalSegments + 1) + x + 1);
    }
  }

  return {std::move(positions), std::move(indices), min, max};
}

void ImplicitSerializer::serializeOctree(
    const Octree& octree,
    const std::filesystem::path& path) {
  SphereGenerator generator;
  auto mesh = generator.generate(glm::vec3(0.0), 3.0);
  auto model = mesh.convertToGltf();

  CesiumGltfWriter::GltfWriter gltfWriter;
  auto glb = gltfWriter.writeGlb(model, model.buffers.front().cesium.data);
  auto glbFile =
      std::fstream(path / "sphere.glb", std::ios::out | std::ios::binary);
  glbFile.write((char*)&glb.gltfBytes[0], glb.gltfBytes.size());

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

  tileset.root.boundingVolume
      .box = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 2.0};
  tileset.root.geometricError = 50.0;
  tileset.root.content.emplace();
  tileset.root.content->uri = "glb/{level}_{x}_{y}_{z}.glb";
  tileset.root.refine = Cesium3DTiles::Tile::Refine::REPLACE;
  auto& implicit =
      tileset.root
          .addExtension<Cesium3DTiles::Extension3dTilesImplicitTiling>();
  implicit.subdivisionScheme =
      Cesium3DTiles::Extension3dTilesImplicitTiling::SubdivisionScheme::OCTREE;
  implicit.subtreeLevels = octree.getSubtreeLevels();
  implicit.availableLevels = octree.getMaxLevels();
  implicit.subtrees.uri = "subtree/{level}/{x}/{y}/{z}.json";

  Cesium3DTilesWriter::TilesetWriter writer;
  auto result = writer.writeTileset(tileset, {true});
  auto tilesetFile =
      std::fstream(path / "tileset.json", std::ios::out | std::ios::binary);
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
  buffer.uri =
      fmt::format(path.stem().string() + "/{}/{}/{}/{}.bin", level, x, y, z);
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
  subtreeAvailabilityView.byteOffset = octreeSubtree._tileAvailable.size() +
                                       octreeSubtree._contentAvailable.size();
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
  auto subtreeFile =
      std::fstream(subtreeFilePath, std::ios::out | std::ios::binary);
  subtreeFile.write((char*)&result.subtreeBytes[0], result.subtreeBytes.size());

  // generate gltf for available tiles
  std::filesystem::create_directories(path.parent_path() / "glb");
  serializeGltf(
      path.parent_path() / "glb",
      octreeSubtree,
      CesiumGeometry::OctreeTileID(level, x, y, z),
      CesiumGeometry::OctreeTileID(level, x, y, z));
}

void ImplicitSerializer::serializeGltf(
    const std::filesystem::path& path,
    const OctreeSubtree& subtree,
    const CesiumGeometry::OctreeTileID& subtreeID,
    const CesiumGeometry::OctreeTileID& octreeID) {
  uint32_t relativeTileLevel = octreeID.level - subtreeID.level;
  if (relativeTileLevel >= subtree._subtreeLevels) {
    return;
  }

  uint64_t relativeTileMortonIdx = libmorton::morton3D_64_encode(
      octreeID.x - (subtreeID.x << relativeTileLevel),
      octreeID.y - (subtreeID.y << relativeTileLevel),
      octreeID.z - (subtreeID.z << relativeTileLevel));

  if (subtree.isTileAvailable(relativeTileLevel, relativeTileMortonIdx)) {
    SphereGenerator generator;
    auto mesh = generator.generate(glm::vec3(0.0), 3.0);
    auto model = mesh.convertToGltf();

    CesiumGltfWriter::GltfWriter gltfWriter;
    auto glb = gltfWriter.writeGlb(model, model.buffers.front().cesium.data);
    auto glbFile = std::fstream(
        path / fmt::format(
                   "{}_{}_{}_{}.glb",
                   octreeID.level,
                   octreeID.x,
                   octreeID.y,
                   octreeID.z),
        std::ios::out | std::ios::binary);
    glbFile.write((char*)&glb.gltfBytes[0], glb.gltfBytes.size());
  }

  for (uint32_t y = 0; y < 2; ++y) {
    for (uint32_t x = 0; x < 2; ++x) {
      for (uint32_t z = 0; z < 2; ++z) {
        CesiumGeometry::OctreeTileID childID(
            octreeID.level + 1,
            octreeID.x + x,
            octreeID.y + y,
            octreeID.z + z);
        serializeGltf(path, subtree, subtreeID, childID);
      }
    }
  }
}
} // namespace Cesium3DTilesSelection
