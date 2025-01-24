#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/calcQuadtreeMaxGeometricError.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumQuantizedMeshTerrain/QuantizedMeshLoader.h>
#include <CesiumUtility/AttributeCompression.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Tracing.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltfContent;
using namespace CesiumUtility;

namespace CesiumQuantizedMeshTerrain {

struct QuantizedMeshHeader {
  // The center of the tile in Earth-centered Fixed coordinates.
  double CenterX;
  double CenterY;
  double CenterZ;

  // The minimum and maximum heights in the area covered by this tile.
  // The minimum may be lower and the maximum may be higher than
  // the height of any vertex in this tile in the case that the min/max vertex
  // was removed during mesh simplification, but these are the appropriate
  // values to use for analysis or visualization.
  float MinimumHeight;
  float MaximumHeight;

  // The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
  // in Earth-centered Fixed coordinates, and the radius is in meters.
  double BoundingSphereCenterX;
  double BoundingSphereCenterY;
  double BoundingSphereCenterZ;
  double BoundingSphereRadius;

  // The horizon occlusion point, expressed in the ellipsoid-scaled
  // Earth-centered Fixed frame. If this point is below the horizon, the entire
  // tile is below the horizon. See
  // http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
  double HorizonOcclusionPointX;
  double HorizonOcclusionPointY;
  double HorizonOcclusionPointZ;

  // The total number of vertices.
  uint32_t vertexCount;
};

enum class QuantizedMeshIndexType { UnsignedShort, UnsignedInt };

struct QuantizedMeshView {
  QuantizedMeshView() noexcept
      : header{nullptr},
        indexType{QuantizedMeshIndexType::UnsignedShort},
        triangleCount{0},
        westEdgeIndicesCount{0},
        southEdgeIndicesCount{0},
        eastEdgeIndicesCount{0},
        northEdgeIndicesCount{0},
        onlyWater{false},
        onlyLand{true},
        metadataJsonLength{0} {}

  const QuantizedMeshHeader* header;

  std::span<const uint16_t> uBuffer;
  std::span<const uint16_t> vBuffer;
  std::span<const uint16_t> heightBuffer;

  QuantizedMeshIndexType indexType;
  uint32_t triangleCount;
  std::span<const std::byte> indicesBuffer;

  uint32_t westEdgeIndicesCount;
  std::span<const std::byte> westEdgeIndicesBuffer;

  uint32_t southEdgeIndicesCount;
  std::span<const std::byte> southEdgeIndicesBuffer;

  uint32_t eastEdgeIndicesCount;
  std::span<const std::byte> eastEdgeIndicesBuffer;

  uint32_t northEdgeIndicesCount;
  std::span<const std::byte> northEdgeIndicesBuffer;

  std::span<const std::byte> octEncodedNormalBuffer;

  bool onlyWater;
  bool onlyLand;

  // water mask will always be a 256*256 map where 0 is land and 255 is water.
  std::span<const std::byte> waterMaskBuffer;

  uint32_t metadataJsonLength;
  std::span<const char> metadataJsonBuffer;
};

// We can't use sizeof(QuantizedMeshHeader) because it may be padded.
constexpr size_t headerLength = 92;
constexpr size_t extensionHeaderLength = 5;

namespace {

int32_t zigZagDecode(int32_t value) noexcept {
  return (value >> 1) ^ (-(value & 1));
}

template <class E, class D>
void decodeIndices(
    const std::span<const E>& encoded,
    const std::span<D>& decoded) {
  if (decoded.size() < encoded.size()) {
    throw std::runtime_error("decoded buffer is too small.");
  }

  E highest = 0;
  for (size_t i = 0; i < encoded.size(); ++i) {
    E code = encoded[i];
    E decodedIdx = static_cast<E>(highest - code);
    decoded[i] = static_cast<D>(decodedIdx);
    if (code == 0) {
      ++highest;
    }
  }
}

template <class T>
T readValue(
    const std::span<const std::byte>& data,
    size_t offset,
    T defaultValue) noexcept {
  if (offset + sizeof(T) <= data.size()) {
    return *reinterpret_cast<const T*>(data.data() + offset);
  }
  return defaultValue;
}

std::optional<QuantizedMeshView> parseQuantizedMesh(
    const std::span<const std::byte>& data,
    bool enableWaterMask) {
  if (data.size() < headerLength) {
    return std::nullopt;
  }

  size_t readIndex = 0;

  // parse header
  QuantizedMeshView meshView;
  meshView.header = reinterpret_cast<const QuantizedMeshHeader*>(data.data());
  readIndex += headerLength;

  // parse u, v, and height buffers
  const uint32_t vertexCount = meshView.header->vertexCount;

  meshView.uBuffer = std::span<const uint16_t>(
      reinterpret_cast<const uint16_t*>(data.data() + readIndex),
      vertexCount);
  readIndex += meshView.uBuffer.size_bytes();
  if (readIndex > data.size()) {
    return std::nullopt;
  }

  meshView.vBuffer = std::span<const uint16_t>(
      reinterpret_cast<const uint16_t*>(data.data() + readIndex),
      vertexCount);
  readIndex += meshView.vBuffer.size_bytes();
  if (readIndex > data.size()) {
    return std::nullopt;
  }

  meshView.heightBuffer = std::span<const uint16_t>(
      reinterpret_cast<const uint16_t*>(data.data() + readIndex),
      vertexCount);
  readIndex += meshView.heightBuffer.size_bytes();
  if (readIndex > data.size()) {
    return std::nullopt;
  }

  // parse the indices buffer
  uint32_t indexSizeBytes;
  if (vertexCount > 65536) {
    // 32-bit indices
    if ((readIndex % 4) != 0) {
      readIndex += 2;
      if (readIndex > data.size()) {
        return std::nullopt;
      }
    }

    meshView.triangleCount = readValue<uint32_t>(data, readIndex, 0);
    readIndex += sizeof(uint32_t);
    if (readIndex > data.size()) {
      return std::nullopt;
    }

    const uint32_t indicesCount = meshView.triangleCount * 3;
    meshView.indicesBuffer = std::span<const std::byte>(
        data.data() + readIndex,
        indicesCount * sizeof(uint32_t));
    readIndex += meshView.indicesBuffer.size_bytes();
    if (readIndex > data.size()) {
      return std::nullopt;
    }

    meshView.indexType = QuantizedMeshIndexType::UnsignedInt;
    indexSizeBytes = sizeof(uint32_t);
  } else {
    // 16-bit indices
    meshView.triangleCount = readValue<uint32_t>(data, readIndex, 0);
    readIndex += sizeof(uint32_t);
    if (readIndex > data.size()) {
      return std::nullopt;
    }

    const uint32_t indicesCount = meshView.triangleCount * 3;
    meshView.indicesBuffer = std::span<const std::byte>(
        data.data() + readIndex,
        indicesCount * sizeof(uint16_t));
    readIndex += meshView.indicesBuffer.size_bytes();
    if (readIndex > data.size()) {
      return std::nullopt;
    }

    meshView.indexType = QuantizedMeshIndexType::UnsignedShort;
    indexSizeBytes = sizeof(uint16_t);
  }

  // prepare to read edge
  size_t edgeByteSizes = 0;

  // read the west edge indices
  meshView.westEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes =
      static_cast<size_t>(meshView.westEdgeIndicesCount * indexSizeBytes);
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.westEdgeIndicesBuffer =
      std::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the south edge
  meshView.southEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes =
      static_cast<size_t>(meshView.southEdgeIndicesCount * indexSizeBytes);
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.southEdgeIndicesBuffer =
      std::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the east edge
  meshView.eastEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes =
      static_cast<size_t>(meshView.eastEdgeIndicesCount * indexSizeBytes);
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.eastEdgeIndicesBuffer =
      std::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the north edge
  meshView.northEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes =
      static_cast<size_t>(meshView.northEdgeIndicesCount * indexSizeBytes);
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.northEdgeIndicesBuffer =
      std::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // parse oct-encoded normal buffer and metadata
  while (readIndex < data.size()) {
    if (readIndex + extensionHeaderLength > data.size()) {
      break;
    }

    const uint8_t extensionID =
        *reinterpret_cast<const uint8_t*>(data.data() + readIndex);
    readIndex += sizeof(uint8_t);
    const uint32_t extensionLength =
        *reinterpret_cast<const uint32_t*>(data.data() + readIndex);
    readIndex += sizeof(uint32_t);

    if (extensionID == 1) {
      // Oct-encoded per-vertex normals
      if (readIndex + static_cast<size_t>(vertexCount * 2) > data.size()) {
        break;
      }

      meshView.octEncodedNormalBuffer = std::span<const std::byte>(
          data.data() + readIndex,
          static_cast<size_t>(vertexCount * 2));
    } else if (enableWaterMask && extensionID == 2) {
      // Water Mask
      if (extensionLength == 1) {
        // Either fully land or fully water
        meshView.onlyWater = static_cast<bool>(
            *reinterpret_cast<const unsigned char*>(data.data() + readIndex));
        meshView.onlyLand = !meshView.onlyWater;
      } else if (extensionLength == 65536) {
        // We have a 256*256 mask defining where the water is within the tile
        // 0 means land, 255 means water
        meshView.onlyWater = false;
        meshView.onlyLand = false;
        meshView.waterMaskBuffer =
            std::span<const std::byte>(data.data() + readIndex, 65536);
      }
    } else if (extensionID == 4) {
      // Metadata
      if (readIndex + sizeof(uint32_t) > data.size()) {
        break;
      }

      meshView.metadataJsonLength =
          *reinterpret_cast<const uint32_t*>(data.data() + readIndex);

      if (readIndex + sizeof(uint32_t) + meshView.metadataJsonLength >
          data.size()) {
        break;
      }

      meshView.metadataJsonBuffer = std::span<const char>(
          reinterpret_cast<const char*>(
              data.data() + sizeof(uint32_t) + readIndex),
          meshView.metadataJsonLength);
    }

    readIndex += extensionLength;
  }

  return meshView;
}

double calculateSkirtHeight(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::GlobeRectangle& rectangle) noexcept {
  const double levelMaximumGeometricError =
      calcQuadtreeMaxGeometricError(ellipsoid) * rectangle.computeWidth();
  return levelMaximumGeometricError * 5.0;
}

template <class E, class I>
void addSkirt(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dvec3& center,
    const CesiumGeospatial::GlobeRectangle& rectangle,
    double minimumHeight,
    double maximumHeight,
    uint32_t currentVertexCount,
    uint32_t currentIndicesCount,
    double skirtHeight,
    double longitudeOffset,
    double latitudeOffset,
    const std::vector<glm::dvec3>& uvsAndHeights,
    const std::span<const E>& edgeIndices,
    const std::span<float>& positions,
    const std::span<float>& normals,
    const std::span<I>& indices,
    glm::dvec3& positionMinimums,
    glm::dvec3& positionMaximums) {
  const double west = rectangle.getWest();
  const double south = rectangle.getSouth();
  const double east = rectangle.getEast();
  const double north = rectangle.getNorth();

  size_t newEdgeIndex = currentVertexCount;
  size_t positionIdx = static_cast<size_t>(currentVertexCount * 3);
  size_t indexIdx = currentIndicesCount;
  for (size_t i = 0; i < edgeIndices.size(); ++i) {
    E edgeIdx = edgeIndices[i];

    const double uRatio = uvsAndHeights[edgeIdx].x;
    const double vRatio = uvsAndHeights[edgeIdx].y;
    const double heightRatio = uvsAndHeights[edgeIdx].z;
    const double longitude = Math::lerp(west, east, uRatio) + longitudeOffset;
    const double latitude = Math::lerp(south, north, vRatio) + latitudeOffset;
    const double heightMeters =
        Math::lerp(minimumHeight, maximumHeight, heightRatio) - skirtHeight;
    glm::dvec3 position = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, heightMeters));
    position -= center;

    positions[positionIdx] = static_cast<float>(position.x);
    positions[positionIdx + 1] = static_cast<float>(position.y);
    positions[positionIdx + 2] = static_cast<float>(position.z);

    positionMinimums = glm::min(positionMinimums, position);
    positionMaximums = glm::max(positionMaximums, position);

    if (!normals.empty()) {
      const size_t componentIndex = static_cast<size_t>(3 * edgeIdx);
      normals[positionIdx] = normals[componentIndex];
      normals[positionIdx + 1] = normals[componentIndex + 1];
      normals[positionIdx + 2] = normals[componentIndex + 2];
    }

    if (i < edgeIndices.size() - 1) {
      E nextEdgeIdx = edgeIndices[i + 1];
      indices[indexIdx++] = static_cast<I>(edgeIdx);
      indices[indexIdx++] = static_cast<I>(nextEdgeIdx);
      indices[indexIdx++] = static_cast<I>(newEdgeIndex);

      indices[indexIdx++] = static_cast<I>(newEdgeIndex);
      indices[indexIdx++] = static_cast<I>(nextEdgeIdx);
      indices[indexIdx++] = static_cast<I>(newEdgeIndex + 1);
    }

    ++newEdgeIndex;
    positionIdx += 3;
  }
}

template <class E, class I>
void addSkirts(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dvec3& center,
    const CesiumGeospatial::GlobeRectangle& rectangle,
    double minimumHeight,
    double maximumHeight,
    uint32_t currentVertexCount,
    uint32_t currentIndicesCount,
    double skirtHeight,
    double longitudeOffset,
    double latitudeOffset,
    const std::vector<glm::dvec3>& uvsAndHeights,
    const std::span<const std::byte>& westEdgeIndicesBuffer,
    const std::span<const std::byte>& southEdgeIndicesBuffer,
    const std::span<const std::byte>& eastEdgeIndicesBuffer,
    const std::span<const std::byte>& northEdgeIndicesBuffer,
    const std::span<float>& outputPositions,
    const std::span<float>& outputNormals,
    const std::span<I>& outputIndices,
    glm::dvec3& positionMinimums,
    glm::dvec3& positionMaximums) {
  const uint32_t westVertexCount =
      static_cast<uint32_t>(westEdgeIndicesBuffer.size() / sizeof(E));
  const uint32_t southVertexCount =
      static_cast<uint32_t>(southEdgeIndicesBuffer.size() / sizeof(E));
  const uint32_t eastVertexCount =
      static_cast<uint32_t>(eastEdgeIndicesBuffer.size() / sizeof(E));
  const uint32_t northVertexCount =
      static_cast<uint32_t>(northEdgeIndicesBuffer.size() / sizeof(E));

  // allocate edge indices to be sort later
  uint32_t maxEdgeVertexCount = westVertexCount;
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, southVertexCount);
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, eastVertexCount);
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, northVertexCount);
  std::vector<E> sortEdgeIndices(maxEdgeVertexCount);

  // add skirt indices, vertices, and normals
  std::span<const E> westEdgeIndices(
      reinterpret_cast<const E*>(westEdgeIndicesBuffer.data()),
      westVertexCount);
  std::partial_sort_copy(
      westEdgeIndices.begin(),
      westEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + westVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) noexcept {
        return uvsAndHeights[lhs].y < uvsAndHeights[rhs].y;
      });
  westEdgeIndices = std::span(sortEdgeIndices.data(), westVertexCount);
  addSkirt(
      ellipsoid,
      center,
      rectangle,
      minimumHeight,
      maximumHeight,
      currentVertexCount,
      currentIndicesCount,
      skirtHeight,
      -longitudeOffset,
      0.0,
      uvsAndHeights,
      westEdgeIndices,
      outputPositions,
      outputNormals,
      outputIndices,
      positionMinimums,
      positionMaximums);

  currentVertexCount += westVertexCount;
  currentIndicesCount += (westVertexCount - 1) * 6;
  std::span<const E> southEdgeIndices(
      reinterpret_cast<const E*>(southEdgeIndicesBuffer.data()),
      southVertexCount);
  std::partial_sort_copy(
      southEdgeIndices.begin(),
      southEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + southVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) noexcept {
        return uvsAndHeights[lhs].x > uvsAndHeights[rhs].x;
      });
  southEdgeIndices = std::span(sortEdgeIndices.data(), southVertexCount);
  addSkirt(
      ellipsoid,
      center,
      rectangle,
      minimumHeight,
      maximumHeight,
      currentVertexCount,
      currentIndicesCount,
      skirtHeight,
      0.0,
      -latitudeOffset,
      uvsAndHeights,
      southEdgeIndices,
      outputPositions,
      outputNormals,
      outputIndices,
      positionMinimums,
      positionMaximums);

  currentVertexCount += southVertexCount;
  currentIndicesCount += (southVertexCount - 1) * 6;
  std::span<const E> eastEdgeIndices(
      reinterpret_cast<const E*>(eastEdgeIndicesBuffer.data()),
      eastVertexCount);
  std::partial_sort_copy(
      eastEdgeIndices.begin(),
      eastEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + eastVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) noexcept {
        return uvsAndHeights[lhs].y > uvsAndHeights[rhs].y;
      });
  eastEdgeIndices = std::span(sortEdgeIndices.data(), eastVertexCount);
  addSkirt(
      ellipsoid,
      center,
      rectangle,
      minimumHeight,
      maximumHeight,
      currentVertexCount,
      currentIndicesCount,
      skirtHeight,
      longitudeOffset,
      0.0,
      uvsAndHeights,
      eastEdgeIndices,
      outputPositions,
      outputNormals,
      outputIndices,
      positionMinimums,
      positionMaximums);

  currentVertexCount += eastVertexCount;
  currentIndicesCount += (eastVertexCount - 1) * 6;
  std::span<const E> northEdgeIndices(
      reinterpret_cast<const E*>(northEdgeIndicesBuffer.data()),
      northVertexCount);
  std::partial_sort_copy(
      northEdgeIndices.begin(),
      northEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + northVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) noexcept {
        return uvsAndHeights[lhs].x < uvsAndHeights[rhs].x;
      });
  northEdgeIndices = std::span(sortEdgeIndices.data(), northVertexCount);
  addSkirt(
      ellipsoid,
      center,
      rectangle,
      minimumHeight,
      maximumHeight,
      currentVertexCount,
      currentIndicesCount,
      skirtHeight,
      0.0,
      latitudeOffset,
      uvsAndHeights,
      northEdgeIndices,
      outputPositions,
      outputNormals,
      outputIndices,
      positionMinimums,
      positionMaximums);
}

static void decodeNormals(
    const std::span<const std::byte>& encoded,
    const std::span<float>& decoded) {
  if (decoded.size() < encoded.size()) {
    throw std::runtime_error("decoded buffer is too small.");
  }

  size_t normalOutputIndex = 0;
  for (size_t i = 0; i < encoded.size(); i += 2) {
    glm::dvec3 normal = AttributeCompression::octDecode(
        static_cast<uint8_t>(encoded[i]),
        static_cast<uint8_t>(encoded[i + 1]));
    decoded[normalOutputIndex++] = static_cast<float>(normal.x);
    decoded[normalOutputIndex++] = static_cast<float>(normal.y);
    decoded[normalOutputIndex++] = static_cast<float>(normal.z);
  }
}

template <class T>
std::vector<std::byte> generateNormals(
    const std::span<const float>& positions,
    const std::span<T>& indices,
    size_t currentNumOfIndex) {
  std::vector<std::byte> normalsBuffer(positions.size() * sizeof(float));
  const std::span<float> normals(
      reinterpret_cast<float*>(normalsBuffer.data()),
      positions.size());
  for (size_t i = 0; i < currentNumOfIndex; i += 3) {
    T id0 = indices[i];
    T id1 = indices[i + 1];
    T id2 = indices[i + 2];
    const size_t id0x3 = static_cast<size_t>(id0) * 3;
    const size_t id1x3 = static_cast<size_t>(id1) * 3;
    const size_t id2x3 = static_cast<size_t>(id2) * 3;

    const glm::vec3 p0 =
        glm::vec3(positions[id0x3], positions[id0x3 + 1], positions[id0x3 + 2]);
    const glm::vec3 p1 =
        glm::vec3(positions[id1x3], positions[id1x3 + 1], positions[id1x3 + 2]);
    const glm::vec3 p2 =
        glm::vec3(positions[id2x3], positions[id2x3 + 1], positions[id2x3 + 2]);

    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    normals[id0x3] += normal.x;
    normals[id0x3 + 1] += normal.y;
    normals[id0x3 + 2] += normal.z;

    normals[id1x3] += normal.x;
    normals[id1x3 + 1] += normal.y;
    normals[id1x3 + 2] += normal.z;

    normals[id2x3] += normal.x;
    normals[id2x3 + 1] += normal.y;
    normals[id2x3 + 2] += normal.z;
  }

  for (size_t i = 0; i < normals.size(); i += 3) {
    glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
    if (!Math::equalsEpsilon(glm::dot(normal, normal), 0.0, Math::Epsilon7)) {
      normal = glm::normalize(normal);
      normals[i] = normal.x;
      normals[i + 1] = normal.y;
      normals[i + 2] = normal.z;
    }
  }

  return normalsBuffer;
}

QuantizedMeshMetadataResult processMetadata(
    const QuadtreeTileID& tileID,
    std::span<const char> metadataString) {
  rapidjson::Document metadata;
  metadata.Parse(
      reinterpret_cast<const char*>(metadataString.data()),
      metadataString.size());

  QuantizedMeshMetadataResult result;

  if (metadata.HasParseError()) {
    result.errors.emplaceError(fmt::format(
        "Error when parsing metadata, error code {} at byte offset {}",
        metadata.GetParseError(),
        metadata.GetErrorOffset()));
    return result;
  }

  return QuantizedMeshLoader::loadAvailabilityRectangles(
      metadata,
      tileID.level + 1);
}
} // namespace

/*static*/ QuantizedMeshLoadResult QuantizedMeshLoader::load(
    const QuadtreeTileID& tileID,
    const BoundingRegion& tileBoundingVolume,
    const std::string& url,
    const std::span<const std::byte>& data,
    bool enableWaterMask,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  CESIUM_TRACE("Cesium3DTilesSelection::QuantizedMeshLoader::load");

  QuantizedMeshLoadResult result;

  std::optional<QuantizedMeshView> meshView =
      parseQuantizedMesh(data, enableWaterMask);
  if (!meshView) {
    result.errors.emplaceError("Unable to parse quantized-mesh-1.0 tile.");
    return result;
  }

  // get vertex count for this mesh
  const QuantizedMeshHeader* pHeader = meshView->header;
  const uint32_t vertexCount = pHeader->vertexCount;
  const uint32_t indicesCount = meshView->triangleCount * 3;
  const uint32_t skirtVertexCount =
      meshView->westEdgeIndicesCount + meshView->southEdgeIndicesCount +
      meshView->eastEdgeIndicesCount + meshView->northEdgeIndicesCount;
  const uint32_t skirtIndicesCount = (skirtVertexCount - 4) * 6;

  // decode position without skirt, but preallocate position buffer to include
  // skirt as well
  std::vector<std::byte> outputPositionsBuffer(
      static_cast<uint64_t>((vertexCount + skirtVertexCount) * 3) *
      sizeof(float));
  std::span<float> outputPositions(
      reinterpret_cast<float*>(outputPositionsBuffer.data()),
      static_cast<size_t>((vertexCount + skirtVertexCount) * 3));
  size_t positionOutputIndex = 0;

  const glm::dvec3 center(
      pHeader->BoundingSphereCenterX,
      pHeader->BoundingSphereCenterY,
      pHeader->BoundingSphereCenterZ);
  const glm::dvec3 horizonOcclusionPoint(
      pHeader->HorizonOcclusionPointX,
      pHeader->HorizonOcclusionPointY,
      pHeader->HorizonOcclusionPointZ);
  const double minimumHeight = pHeader->MinimumHeight;
  const double maximumHeight = pHeader->MaximumHeight;

  glm::dvec3 positionMinimums{std::numeric_limits<double>::max()};
  glm::dvec3 positionMaximums{std::numeric_limits<double>::lowest()};

  const CesiumGeospatial::GlobeRectangle& rectangle =
      tileBoundingVolume.getRectangle();
  const double west = rectangle.getWest();
  const double south = rectangle.getSouth();
  const double east = rectangle.getEast();
  const double north = rectangle.getNorth();

  int32_t u = 0;
  int32_t v = 0;
  int32_t height = 0;
  std::vector<glm::dvec3> uvsAndHeights;
  uvsAndHeights.reserve(vertexCount);
  for (size_t i = 0; i < vertexCount; ++i) {
    u += zigZagDecode(meshView->uBuffer[i]);
    v += zigZagDecode(meshView->vBuffer[i]);
    height += zigZagDecode(meshView->heightBuffer[i]);

    double uRatio = static_cast<double>(u) / 32767.0;
    double vRatio = static_cast<double>(v) / 32767.0;
    double heightRatio = static_cast<double>(height) / 32767.0;

    const double longitude = Math::lerp(west, east, uRatio);
    const double latitude = Math::lerp(south, north, vRatio);
    const double heightMeters =
        Math::lerp(minimumHeight, maximumHeight, heightRatio);

    glm::dvec3 position = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, heightMeters));
    position -= center;
    outputPositions[positionOutputIndex++] = static_cast<float>(position.x);
    outputPositions[positionOutputIndex++] = static_cast<float>(position.y);
    outputPositions[positionOutputIndex++] = static_cast<float>(position.z);

    positionMinimums = glm::min(positionMinimums, position);
    positionMaximums = glm::max(positionMaximums, position);

    uvsAndHeights.emplace_back(uRatio, vRatio, heightRatio);
  }

  // decode normal vertices of the tile as well as its metadata without skirt
  std::vector<std::byte> outputNormalsBuffer;
  std::span<float> outputNormals;
  if (!meshView->octEncodedNormalBuffer.empty()) {
    const uint32_t totalNormalFloats = (vertexCount + skirtVertexCount) * 3;
    outputNormalsBuffer.resize(totalNormalFloats * sizeof(float));
    outputNormals = std::span<float>(
        reinterpret_cast<float*>(outputNormalsBuffer.data()),
        totalNormalFloats);
    decodeNormals(meshView->octEncodedNormalBuffer, outputNormals);

    outputNormals = std::span<float>(
        reinterpret_cast<float*>(outputNormalsBuffer.data()),
        outputNormalsBuffer.size() / sizeof(float));
  }

  // decode metadata
  if (meshView->metadataJsonLength > 0) {
    QuantizedMeshMetadataResult metadata =
        processMetadata(tileID, meshView->metadataJsonBuffer);
    result.availableTileRectangles = std::move(metadata.availability);
    result.errors.merge(std::move(metadata.errors));
  }

  // indices buffer for gltf to include tile and skirt indices. Caution of
  // indices type since adding skirt means the number of vertices is potentially
  // over maximum of uint16_t
  std::vector<std::byte> outputIndicesBuffer;
  uint32_t indexSizeBytes =
      meshView->indexType == QuantizedMeshIndexType::UnsignedInt
          ? sizeof(uint32_t)
          : sizeof(uint16_t);
  const double skirtHeight = calculateSkirtHeight(ellipsoid, rectangle);
  const double longitudeOffset = (east - west) * 0.0001;
  const double latitudeOffset = (north - south) * 0.0001;
  if (meshView->indexType == QuantizedMeshIndexType::UnsignedInt) {
    // decode the tile indices without skirt.
    const size_t outputIndicesCount = indicesCount + skirtIndicesCount;
    outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint32_t));
    const std::span<const uint32_t> indices(
        reinterpret_cast<const uint32_t*>(meshView->indicesBuffer.data()),
        indicesCount);
    std::span<uint32_t> outputIndices(
        reinterpret_cast<uint32_t*>(outputIndicesBuffer.data()),
        outputIndicesCount);
    decodeIndices(indices, outputIndices);

    // generate normals if no provided
    if (outputNormalsBuffer.empty()) {
      outputNormalsBuffer =
          generateNormals(outputPositions, outputIndices, indicesCount);
      outputNormals = std::span<float>(
          reinterpret_cast<float*>(outputNormalsBuffer.data()),
          outputNormalsBuffer.size() / sizeof(float));
    }

    // add skirt
    addSkirts<uint32_t, uint32_t>(
        ellipsoid,
        center,
        rectangle,
        minimumHeight,
        maximumHeight,
        vertexCount,
        indicesCount,
        skirtHeight,
        longitudeOffset,
        latitudeOffset,
        uvsAndHeights,
        meshView->westEdgeIndicesBuffer,
        meshView->southEdgeIndicesBuffer,
        meshView->eastEdgeIndicesBuffer,
        meshView->northEdgeIndicesBuffer,
        outputPositions,
        outputNormals,
        outputIndices,
        positionMinimums,
        positionMaximums);

    indexSizeBytes = sizeof(uint32_t);
  } else {
    const size_t outputIndicesCount = indicesCount + skirtIndicesCount;
    const std::span<const uint16_t> indices(
        reinterpret_cast<const uint16_t*>(meshView->indicesBuffer.data()),
        indicesCount);
    if (vertexCount + skirtVertexCount < std::numeric_limits<uint16_t>::max()) {
      // decode the tile indices without skirt.
      outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint16_t));
      std::span<uint16_t> outputIndices(
          reinterpret_cast<uint16_t*>(outputIndicesBuffer.data()),
          outputIndicesCount);
      decodeIndices(indices, outputIndices);

      // generate normals if no provided
      if (outputNormalsBuffer.empty()) {
        outputNormalsBuffer =
            generateNormals(outputPositions, outputIndices, indicesCount);
        outputNormals = std::span<float>(
            reinterpret_cast<float*>(outputNormalsBuffer.data()),
            outputNormalsBuffer.size() / sizeof(float));
      }

      addSkirts<uint16_t, uint16_t>(
          ellipsoid,
          center,
          rectangle,
          minimumHeight,
          maximumHeight,
          vertexCount,
          indicesCount,
          skirtHeight,
          longitudeOffset,
          latitudeOffset,
          uvsAndHeights,
          meshView->westEdgeIndicesBuffer,
          meshView->southEdgeIndicesBuffer,
          meshView->eastEdgeIndicesBuffer,
          meshView->northEdgeIndicesBuffer,
          outputPositions,
          outputNormals,
          outputIndices,
          positionMinimums,
          positionMaximums);

      indexSizeBytes = sizeof(uint16_t);
    } else {
      outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint32_t));
      std::span<uint32_t> outputIndices(
          reinterpret_cast<uint32_t*>(outputIndicesBuffer.data()),
          outputIndicesCount);
      decodeIndices(indices, outputIndices);

      // generate normals if no provided
      if (outputNormalsBuffer.empty()) {
        outputNormalsBuffer =
            generateNormals(outputPositions, outputIndices, indicesCount);
        outputNormals = std::span<float>(
            reinterpret_cast<float*>(outputNormalsBuffer.data()),
            outputNormalsBuffer.size() / sizeof(float));
      }

      addSkirts<uint16_t, uint32_t>(
          ellipsoid,
          center,
          rectangle,
          minimumHeight,
          maximumHeight,
          vertexCount,
          indicesCount,
          skirtHeight,
          longitudeOffset,
          latitudeOffset,
          uvsAndHeights,
          meshView->westEdgeIndicesBuffer,
          meshView->southEdgeIndicesBuffer,
          meshView->eastEdgeIndicesBuffer,
          meshView->northEdgeIndicesBuffer,
          outputPositions,
          outputNormals,
          outputIndices,
          positionMinimums,
          positionMaximums);

      indexSizeBytes = sizeof(uint32_t);
    }
  }

  // create gltf
  CesiumGltf::Model& model = result.model.emplace();

  model.asset.version = "2.0";

  CesiumGltf::Material& material = model.materials.emplace_back();
  CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness.emplace();
  pbr.metallicFactor = 0.0;
  pbr.roughnessFactor = 1.0;

  const size_t meshId = model.meshes.size();
  model.meshes.emplace_back();
  CesiumGltf::Mesh& mesh = model.meshes[meshId];
  mesh.primitives.emplace_back();

  CesiumGltf::MeshPrimitive& primitive = mesh.primitives[0];
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
  primitive.material = 0;

  // add position buffer to gltf
  const size_t positionBufferId = model.buffers.size();
  model.buffers.emplace_back();
  CesiumGltf::Buffer& positionBuffer = model.buffers[positionBufferId];
  positionBuffer.byteLength = int64_t(outputPositionsBuffer.size());
  positionBuffer.cesium.data = std::move(outputPositionsBuffer);

  const size_t positionBufferViewId = model.bufferViews.size();
  model.bufferViews.emplace_back();
  CesiumGltf::BufferView& positionBufferView =
      model.bufferViews[positionBufferViewId];
  positionBufferView.buffer = int32_t(positionBufferId);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteStride = 3 * sizeof(float);
  positionBufferView.byteLength = int64_t(positionBuffer.cesium.data.size());
  positionBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  const size_t positionAccessorId = model.accessors.size();
  model.accessors.emplace_back();
  CesiumGltf::Accessor& positionAccessor = model.accessors[positionAccessorId];
  positionAccessor.bufferView = static_cast<int>(positionBufferViewId);
  positionAccessor.byteOffset = 0;
  positionAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  positionAccessor.count = vertexCount + skirtVertexCount;
  positionAccessor.type = CesiumGltf::Accessor::Type::VEC3;
  positionAccessor.min = {
      positionMinimums.x,
      positionMinimums.y,
      positionMinimums.z};
  positionAccessor.max = {
      positionMaximums.x,
      positionMaximums.y,
      positionMaximums.z};

  primitive.attributes.emplace("POSITION", int32_t(positionAccessorId));

  // add normal buffer to gltf if there are any
  if (!outputNormalsBuffer.empty()) {
    const size_t normalBufferId = model.buffers.size();
    model.buffers.emplace_back();
    CesiumGltf::Buffer& normalBuffer = model.buffers[normalBufferId];
    normalBuffer.byteLength = int64_t(outputNormalsBuffer.size());
    normalBuffer.cesium.data = std::move(outputNormalsBuffer);

    const size_t normalBufferViewId = model.bufferViews.size();
    model.bufferViews.emplace_back();
    CesiumGltf::BufferView& normalBufferView =
        model.bufferViews[normalBufferViewId];
    normalBufferView.buffer = int32_t(normalBufferId);
    normalBufferView.byteOffset = 0;
    normalBufferView.byteStride = 3 * sizeof(float);
    normalBufferView.byteLength = int64_t(normalBuffer.cesium.data.size());
    normalBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    const size_t normalAccessorId = model.accessors.size();
    model.accessors.emplace_back();
    CesiumGltf::Accessor& normalAccessor = model.accessors[normalAccessorId];
    normalAccessor.bufferView = int32_t(normalBufferViewId);
    normalAccessor.byteOffset = 0;
    normalAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
    normalAccessor.count = vertexCount + skirtVertexCount;
    normalAccessor.type = CesiumGltf::Accessor::Type::VEC3;

    primitive.attributes.emplace("NORMAL", static_cast<int>(normalAccessorId));
  }

  // add indices buffer to gltf
  const size_t indicesBufferId = model.buffers.size();
  model.buffers.emplace_back();
  CesiumGltf::Buffer& indicesBuffer = model.buffers[indicesBufferId];
  indicesBuffer.byteLength = int64_t(outputIndicesBuffer.size());
  indicesBuffer.cesium.data = std::move(outputIndicesBuffer);

  const size_t indicesBufferViewId = model.bufferViews.size();
  model.bufferViews.emplace_back();
  CesiumGltf::BufferView& indicesBufferView =
      model.bufferViews[indicesBufferViewId];
  indicesBufferView.buffer = int32_t(indicesBufferId);
  indicesBufferView.byteOffset = 0;
  indicesBufferView.byteLength = int64_t(indicesBuffer.cesium.data.size());
  indicesBufferView.target =
      CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER;

  const size_t indicesAccessorId = model.accessors.size();
  model.accessors.emplace_back();
  CesiumGltf::Accessor& indicesAccessor = model.accessors[indicesAccessorId];
  indicesAccessor.bufferView = int32_t(indicesBufferViewId);
  indicesAccessor.byteOffset = 0;
  indicesAccessor.type = CesiumGltf::Accessor::Type::SCALAR;
  indicesAccessor.count = indicesCount + skirtIndicesCount;
  indicesAccessor.componentType =
      indexSizeBytes == sizeof(uint32_t)
          ? CesiumGltf::Accessor::ComponentType::UNSIGNED_INT
          : CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT;

  primitive.indices = int32_t(indicesBufferId);

  // add skirts info to primitive extra in case we need to upsample from it
  SkirtMeshMetadata skirtMeshMetadata;
  skirtMeshMetadata.noSkirtIndicesBegin = 0;
  skirtMeshMetadata.noSkirtIndicesCount = indicesCount;
  skirtMeshMetadata.noSkirtVerticesBegin = 0;
  skirtMeshMetadata.noSkirtVerticesCount = vertexCount;
  skirtMeshMetadata.meshCenter = center;
  skirtMeshMetadata.skirtWestHeight = skirtHeight;
  skirtMeshMetadata.skirtSouthHeight = skirtHeight;
  skirtMeshMetadata.skirtEastHeight = skirtHeight;
  skirtMeshMetadata.skirtNorthHeight = skirtHeight;

  primitive.extras = SkirtMeshMetadata::createGltfExtras(skirtMeshMetadata);

  // add only-water and only-land flags to primitive extras
  primitive.extras.emplace("OnlyWater", meshView->onlyWater);
  primitive.extras.emplace("OnlyLand", meshView->onlyLand);

  // TODO: use KHR_texture_transform
  primitive.extras.emplace("WaterMaskTranslationX", 0.0);
  primitive.extras.emplace("WaterMaskTranslationY", 0.0);
  primitive.extras.emplace("WaterMaskScale", 1.0);

  // if there is a combination of water and land, add the full water mask
  if (!meshView->onlyWater && !meshView->onlyLand) {

    // create source image
    const size_t waterMaskImageId = model.images.size();
    model.images.emplace_back();
    CesiumGltf::Image& waterMaskImage = model.images[waterMaskImageId];
    waterMaskImage.pAsset.emplace();
    waterMaskImage.pAsset->width = 256;
    waterMaskImage.pAsset->height = 256;
    waterMaskImage.pAsset->channels = 1;
    waterMaskImage.pAsset->bytesPerChannel = 1;
    waterMaskImage.pAsset->pixelData.resize(65536);
    std::memcpy(
        waterMaskImage.pAsset->pixelData.data(),
        meshView->waterMaskBuffer.data(),
        65536);

    // create sampler parameters
    const size_t waterMaskSamplerId = model.samplers.size();
    model.samplers.emplace_back();
    CesiumGltf::Sampler& waterMaskSampler = model.samplers[waterMaskSamplerId];
    waterMaskSampler.magFilter = CesiumGltf::Sampler::MagFilter::LINEAR;
    waterMaskSampler.minFilter =
        CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST;
    waterMaskSampler.wrapS = CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE;
    waterMaskSampler.wrapT = CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE;

    // create texture
    const size_t waterMaskTextureId = model.textures.size();
    model.textures.emplace_back();
    CesiumGltf::Texture& waterMaskTexture = model.textures[waterMaskTextureId];
    waterMaskTexture.sampler = int32_t(waterMaskSamplerId);
    waterMaskTexture.source = int32_t(waterMaskImageId);

    // store the texture id in the extras
    primitive.extras.emplace("WaterMaskTex", int32_t(waterMaskTextureId));
  } else {
    primitive.extras.emplace("WaterMaskTex", int32_t(-1));
  }

  // create node and update bounding volume
  CesiumGltf::Node& node = model.nodes.emplace_back();
  node.mesh = 0;
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGltf::Scene& scene = model.scenes.emplace_back();
  scene.nodes.emplace_back(0);

  model.scene = 0;

  result.updatedBoundingVolume =
      BoundingRegion(rectangle, minimumHeight, maximumHeight, ellipsoid);

  if (result.model) {
    result.model->extras["Cesium3DTiles_TileUrl"] = url;
  }

  return result;
}

QuantizedMeshMetadataResult QuantizedMeshLoader::loadAvailabilityRectangles(
    const rapidjson::Document& metadata,
    uint32_t startingLevel) {
  QuantizedMeshMetadataResult ret;
  const auto availableIt = metadata.FindMember("available");
  if (availableIt == metadata.MemberEnd() || !availableIt->value.IsArray()) {
    return ret;
  }

  const auto& available = availableIt->value;
  if (available.Size() == 0) {
    return ret;
  }

  for (rapidjson::SizeType i = 0; i < available.Size(); ++i) {
    const auto& rangesAtLevelJson = available[i];
    if (!rangesAtLevelJson.IsArray()) {
      continue;
    }

    for (rapidjson::SizeType j = 0; j < rangesAtLevelJson.Size(); ++j) {
      const auto& rangeJson = rangesAtLevelJson[j];
      if (!rangeJson.IsObject()) {
        continue;
      }

      ret.availability.emplace_back(
          CesiumGeometry::QuadtreeTileRectangularRange{
              startingLevel,
              JsonHelpers::getUint32OrDefault(rangeJson, "startX", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "startY", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "endX", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "endY", 0)});
    }

    ++startingLevel;
  }
  return ret;
}

struct TileRange {
  uint32_t minimumX;
  uint32_t minimumY;
  uint32_t maximumX;
  uint32_t maximumY;
};

/*static*/ QuantizedMeshMetadataResult QuantizedMeshLoader::loadMetadata(
    const std::span<const std::byte>& data,
    const QuadtreeTileID& tileID) {
  std::optional<QuantizedMeshView> meshView = parseQuantizedMesh(data, false);
  if (!meshView) {
    return {};
  }
  return processMetadata(tileID, meshView->metadataJsonBuffer);
}

} // namespace CesiumQuantizedMeshTerrain