#include "QuantizedMeshContent.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumGeometry/QuadtreeTileRectangularRange.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/Uri.h"
#include "SkirtMeshMetadata.h"
#include "calcQuadtreeMaxGeometricError.h"
#include <cstddef>
#include <glm/vec3.hpp>
#include <rapidjson/document.h>
#include <stdexcept>

using namespace CesiumUtility;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;

namespace Cesium3DTiles {
/*static*/ std::string QuantizedMeshContent::CONTENT_TYPE =
    "application/vnd.quantized-mesh";

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

struct ExtensionHeader {
  uint8_t extensionId;
  uint32_t extensionLength;
};

enum class QuantizedMeshIndexType { UnsignedShort, UnsignedInt };

struct QuantizedMeshView {
  QuantizedMeshView()
      : header{nullptr},
        indexType{QuantizedMeshIndexType::UnsignedShort},
        triangleCount{0},
        westEdgeIndicesCount{0},
        southEdgeIndicesCount{0},
        eastEdgeIndicesCount{0},
        northEdgeIndicesCount{0},
        metadataJsonLength{0} {}

  const QuantizedMeshHeader* header;

  gsl::span<const uint16_t> uBuffer;
  gsl::span<const uint16_t> vBuffer;
  gsl::span<const uint16_t> heightBuffer;

  QuantizedMeshIndexType indexType;
  uint32_t triangleCount;
  gsl::span<const std::byte> indicesBuffer;

  uint32_t westEdgeIndicesCount;
  gsl::span<const std::byte> westEdgeIndicesBuffer;

  uint32_t southEdgeIndicesCount;
  gsl::span<const std::byte> southEdgeIndicesBuffer;

  uint32_t eastEdgeIndicesCount;
  gsl::span<const std::byte> eastEdgeIndicesBuffer;

  uint32_t northEdgeIndicesCount;
  gsl::span<const std::byte> northEdgeIndicesBuffer;

  gsl::span<const std::byte> octEncodedNormalBuffer;

  uint32_t metadataJsonLength;
  gsl::span<const char> metadataJsonBuffer;
};

// We can't use sizeof(QuantizedMeshHeader) because it may be padded.
const size_t headerLength = 92;
const size_t extensionHeaderLength = 5;

int32_t zigZagDecode(int32_t value) { return (value >> 1) ^ (-(value & 1)); }

template <class E, class D>
void decodeIndices(const gsl::span<const E>& encoded, gsl::span<D>& decoded) {
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
static T readValue(
    const gsl::span<const std::byte>& data,
    size_t offset,
    T defaultValue) {
  if (offset + sizeof(T) <= data.size()) {
    return *reinterpret_cast<const T*>(data.data() + offset);
  }
  return defaultValue;
}

static glm::dvec3 octDecode(uint8_t x, uint8_t y) {
  const uint8_t rangeMax = 255;

  glm::dvec3 result;

  result.x = CesiumUtility::Math::fromSNorm(x, rangeMax);
  result.y = CesiumUtility::Math::fromSNorm(y, rangeMax);
  result.z = 1.0 - (glm::abs(result.x) + glm::abs(result.y));

  if (result.z < 0.0) {
    double oldVX = result.x;
    result.x =
        (1.0 - glm::abs(result.y)) * CesiumUtility::Math::signNotZero(oldVX);
    result.y =
        (1.0 - glm::abs(oldVX)) * CesiumUtility::Math::signNotZero(result.y);
  }

  return glm::normalize(result);
}

static void processMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const QuadtreeTileID& tileID,
    gsl::span<const char> json,
    TileContentLoadResult& result);

static std::optional<QuantizedMeshView>
parseQuantizedMesh(const gsl::span<const std::byte>& data) {
  if (data.size() < headerLength) {
    return std::nullopt;
  }

  size_t readIndex = 0;

  // parse header
  QuantizedMeshView meshView;
  meshView.header = reinterpret_cast<const QuantizedMeshHeader*>(data.data());
  readIndex += headerLength;

  // parse u, v, and height buffers
  uint32_t vertexCount = meshView.header->vertexCount;

  meshView.uBuffer = gsl::span<const uint16_t>(
      reinterpret_cast<const uint16_t*>(data.data() + readIndex),
      vertexCount);
  readIndex += meshView.uBuffer.size_bytes();
  if (readIndex > data.size()) {
    return std::nullopt;
  }

  meshView.vBuffer = gsl::span<const uint16_t>(
      reinterpret_cast<const uint16_t*>(data.data() + readIndex),
      vertexCount);
  readIndex += meshView.vBuffer.size_bytes();
  if (readIndex > data.size()) {
    return std::nullopt;
  }

  meshView.heightBuffer = gsl::span<const uint16_t>(
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

    uint32_t indicesCount = meshView.triangleCount * 3;
    meshView.indicesBuffer = gsl::span<const std::byte>(
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

    uint32_t indicesCount = meshView.triangleCount * 3;
    meshView.indicesBuffer = gsl::span<const std::byte>(
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
  edgeByteSizes = meshView.westEdgeIndicesCount * indexSizeBytes;
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.westEdgeIndicesBuffer =
      gsl::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the south edge
  meshView.southEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes = meshView.southEdgeIndicesCount * indexSizeBytes;
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.southEdgeIndicesBuffer =
      gsl::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the east edge
  meshView.eastEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes = meshView.eastEdgeIndicesCount * indexSizeBytes;
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.eastEdgeIndicesBuffer =
      gsl::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // read the north edge
  meshView.northEdgeIndicesCount = readValue<uint32_t>(data, readIndex, 0);
  readIndex += sizeof(uint32_t);
  edgeByteSizes = meshView.northEdgeIndicesCount * indexSizeBytes;
  if (readIndex + edgeByteSizes > data.size()) {
    return std::nullopt;
  }

  meshView.northEdgeIndicesBuffer =
      gsl::span<const std::byte>(data.data() + readIndex, edgeByteSizes);
  readIndex += edgeByteSizes;

  // parse oct-encoded normal buffer and metadata
  while (readIndex < data.size()) {
    if (readIndex + extensionHeaderLength > data.size()) {
      break;
    }

    uint8_t extensionID =
        *reinterpret_cast<const uint8_t*>(data.data() + readIndex);
    readIndex += sizeof(uint8_t);
    uint32_t extensionLength =
        *reinterpret_cast<const uint32_t*>(data.data() + readIndex);
    readIndex += sizeof(uint32_t);

    if (extensionID == 1) {
      // Oct-encoded per-vertex normals
      if (readIndex + vertexCount * 2 > data.size()) {
        break;
      }

      meshView.octEncodedNormalBuffer =
          gsl::span<const std::byte>(data.data() + readIndex, vertexCount * 2);
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

      meshView.metadataJsonBuffer = gsl::span<const char>(
          reinterpret_cast<const char*>(
              data.data() + sizeof(uint32_t) + readIndex),
          meshView.metadataJsonLength);
    }

    readIndex += extensionLength;
  }

  return meshView;
}

static double calculateSkirtHeight(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::GlobeRectangle& rectangle) {
  double levelMaximumGeometricError =
      calcQuadtreeMaxGeometricError(ellipsoid) * rectangle.computeWidth();
  return levelMaximumGeometricError * 5.0;
}

template <class E, class I>
static void addSkirt(
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
    const gsl::span<const E>& edgeIndices,
    gsl::span<float>& positions,
    gsl::span<float>& normals,
    gsl::span<I>& indices) {
  double west = rectangle.getWest();
  double south = rectangle.getSouth();
  double east = rectangle.getEast();
  double north = rectangle.getNorth();

  size_t newEdgeIndex = currentVertexCount;
  size_t positionIdx = currentVertexCount * 3;
  size_t indexIdx = currentIndicesCount;
  for (size_t i = 0; i < edgeIndices.size(); ++i) {
    E edgeIdx = edgeIndices[i];

    double uRatio = uvsAndHeights[edgeIdx].x;
    double vRatio = uvsAndHeights[edgeIdx].y;
    double heightRatio = uvsAndHeights[edgeIdx].z;
    double longitude = Math::lerp(west, east, uRatio) + longitudeOffset;
    double latitude = Math::lerp(south, north, vRatio) + latitudeOffset;
    double heightMeters =
        Math::lerp(minimumHeight, maximumHeight, heightRatio) - skirtHeight;
    glm::dvec3 position = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, heightMeters));
    position -= center;

    positions[positionIdx] = static_cast<float>(position.x);
    positions[positionIdx + 1] = static_cast<float>(position.y);
    positions[positionIdx + 2] = static_cast<float>(position.z);

    if (!normals.empty()) {
      size_t componentIndex = static_cast<size_t>(3 * edgeIdx);
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
static void addSkirts(
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
    const gsl::span<const std::byte>& westEdgeIndicesBuffer,
    const gsl::span<const std::byte>& southEdgeIndicesBuffer,
    const gsl::span<const std::byte>& eastEdgeIndicesBuffer,
    const gsl::span<const std::byte>& northEdgeIndicesBuffer,
    gsl::span<float>& outputPositions,
    gsl::span<float>& outputNormals,
    gsl::span<I>& outputIndices) {
  uint32_t westVertexCount =
      static_cast<uint32_t>(westEdgeIndicesBuffer.size() / sizeof(E));
  uint32_t southVertexCount =
      static_cast<uint32_t>(southEdgeIndicesBuffer.size() / sizeof(E));
  uint32_t eastVertexCount =
      static_cast<uint32_t>(eastEdgeIndicesBuffer.size() / sizeof(E));
  uint32_t northVertexCount =
      static_cast<uint32_t>(northEdgeIndicesBuffer.size() / sizeof(E));

  // allocate edge indices to be sort later
  uint32_t maxEdgeVertexCount = westVertexCount;
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, southVertexCount);
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, eastVertexCount);
  maxEdgeVertexCount = glm::max(maxEdgeVertexCount, northVertexCount);
  std::vector<E> sortEdgeIndices(maxEdgeVertexCount);

  // add skirt indices, vertices, and normals
  gsl::span<const E> westEdgeIndices(
      reinterpret_cast<const E*>(westEdgeIndicesBuffer.data()),
      westVertexCount);
  std::partial_sort_copy(
      westEdgeIndices.begin(),
      westEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + westVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) {
        return uvsAndHeights[lhs].y < uvsAndHeights[rhs].y;
      });
  westEdgeIndices = gsl::span(sortEdgeIndices.data(), westVertexCount);
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
      outputIndices);

  currentVertexCount += westVertexCount;
  currentIndicesCount += (westVertexCount - 1) * 6;
  gsl::span<const E> southEdgeIndices(
      reinterpret_cast<const E*>(southEdgeIndicesBuffer.data()),
      southVertexCount);
  std::partial_sort_copy(
      southEdgeIndices.begin(),
      southEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + southVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) {
        return uvsAndHeights[lhs].x > uvsAndHeights[rhs].x;
      });
  southEdgeIndices = gsl::span(sortEdgeIndices.data(), southVertexCount);
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
      outputIndices);

  currentVertexCount += southVertexCount;
  currentIndicesCount += (southVertexCount - 1) * 6;
  gsl::span<const E> eastEdgeIndices(
      reinterpret_cast<const E*>(eastEdgeIndicesBuffer.data()),
      eastVertexCount);
  std::partial_sort_copy(
      eastEdgeIndices.begin(),
      eastEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + eastVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) {
        return uvsAndHeights[lhs].y > uvsAndHeights[rhs].y;
      });
  eastEdgeIndices = gsl::span(sortEdgeIndices.data(), eastVertexCount);
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
      outputIndices);

  currentVertexCount += eastVertexCount;
  currentIndicesCount += (eastVertexCount - 1) * 6;
  gsl::span<const E> northEdgeIndices(
      reinterpret_cast<const E*>(northEdgeIndicesBuffer.data()),
      northVertexCount);
  std::partial_sort_copy(
      northEdgeIndices.begin(),
      northEdgeIndices.end(),
      sortEdgeIndices.begin(),
      sortEdgeIndices.begin() + northVertexCount,
      [&uvsAndHeights](auto lhs, auto rhs) {
        return uvsAndHeights[lhs].x < uvsAndHeights[rhs].x;
      });
  northEdgeIndices = gsl::span(sortEdgeIndices.data(), northVertexCount);
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
      outputIndices);
}

static void decodeNormals(
    const gsl::span<const std::byte>& encoded,
    gsl::span<float>& decoded) {
  if (decoded.size() < encoded.size()) {
    throw std::runtime_error("decoded buffer is too small.");
  }

  size_t normalOutputIndex = 0;
  for (size_t i = 0; i < encoded.size(); i += 2) {
    glm::dvec3 normal = octDecode(
        static_cast<uint8_t>(encoded[i]),
        static_cast<uint8_t>(encoded[i + 1]));
    decoded[normalOutputIndex++] = static_cast<float>(normal.x);
    decoded[normalOutputIndex++] = static_cast<float>(normal.y);
    decoded[normalOutputIndex++] = static_cast<float>(normal.z);
  }
}

template <class T>
static std::vector<std::byte> generateNormals(
    const gsl::span<const float>& positions,
    const gsl::span<T>& indices,
    size_t currentNumOfIndex) {
  std::vector<std::byte> normalsBuffer(positions.size() * sizeof(float));
  gsl::span<float> normals(
      reinterpret_cast<float*>(normalsBuffer.data()),
      positions.size());
  for (size_t i = 0; i < currentNumOfIndex; i += 3) {
    T id0 = indices[i];
    T id1 = indices[i + 1];
    T id2 = indices[i + 2];
    size_t id0x3 = static_cast<size_t>(id0) * 3;
    size_t id1x3 = static_cast<size_t>(id1) * 3;
    size_t id2x3 = static_cast<size_t>(id2) * 3;

    glm::vec3 p0 =
        glm::vec3(positions[id0x3], positions[id0x3 + 1], positions[id0x3 + 2]);
    glm::vec3 p1 =
        glm::vec3(positions[id1x3], positions[id1x3 + 1], positions[id1x3 + 2]);
    glm::vec3 p2 =
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
    if (!Math::equalsEpsilon(glm::dot(normal, normal), 0.0, Math::EPSILON7)) {
      normal = glm::normalize(normal);
      normals[i] = normal.x;
      normals[i + 1] = normal.y;
      normals[i + 2] = normal.z;
    }
  }

  return normalsBuffer;
}

std::unique_ptr<TileContentLoadResult>
QuantizedMeshContent::load(const TileContentLoadInput& input) {
  return load(
      input.pLogger,
      input.tileID,
      input.tileBoundingVolume,
      input.url,
      input.data);
}

/*static*/ std::unique_ptr<TileContentLoadResult> QuantizedMeshContent::load(
    std::shared_ptr<spdlog::logger> pLogger,
    const TileID& tileID,
    const BoundingVolume& tileBoundingVolume,
    const std::string& url,
    const gsl::span<const std::byte>& data) {
  // TODO: use context plus tileID to compute the tile's rectangle, rather than
  // inferring it from the parent tile.
  const QuadtreeTileID& id = std::get<QuadtreeTileID>(tileID);

  std::unique_ptr<TileContentLoadResult> pResult =
      std::make_unique<TileContentLoadResult>();
  std::optional<QuantizedMeshView> meshView = parseQuantizedMesh(data);
  if (!meshView) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Unable to parse quantized-mesh-1.0 tile {}.",
        url);
    return pResult;
  }

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&tileBoundingVolume);
  if (!pRegion) {
    const BoundingRegionWithLooseFittingHeights* pLooseRegion =
        std::get_if<BoundingRegionWithLooseFittingHeights>(&tileBoundingVolume);
    if (pLooseRegion) {
      pRegion = &pLooseRegion->getBoundingRegion();
    }
  }

  if (!pRegion) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Unable to create quantized-mesh-1.0 tile {} because the tile's "
        "bounding volume is not a bounding region.",
        url);
    return pResult;
  }

  // get vertex count for this mesh
  const QuantizedMeshHeader* pHeader = meshView->header;
  uint32_t vertexCount = pHeader->vertexCount;
  uint32_t indicesCount = meshView->triangleCount * 3;
  uint32_t skirtVertexCount =
      meshView->westEdgeIndicesCount + meshView->southEdgeIndicesCount +
      meshView->eastEdgeIndicesCount + meshView->northEdgeIndicesCount;
  uint32_t skirtIndicesCount = (skirtVertexCount - 4) * 6;

  // decode position without skirt, but preallocate position buffer to include
  // skirt as well
  std::vector<std::byte> outputPositionsBuffer(
      (vertexCount + skirtVertexCount) * 3 * sizeof(float));
  gsl::span<float> outputPositions(
      reinterpret_cast<float*>(outputPositionsBuffer.data()),
      (vertexCount + skirtVertexCount) * 3);
  size_t positionOutputIndex = 0;

  glm::dvec3 center(
      pHeader->BoundingSphereCenterX,
      pHeader->BoundingSphereCenterY,
      pHeader->BoundingSphereCenterZ);
  glm::dvec3 horizonOcclusionPoint(
      pHeader->HorizonOcclusionPointX,
      pHeader->HorizonOcclusionPointY,
      pHeader->HorizonOcclusionPointZ);
  double minimumHeight = pHeader->MinimumHeight;
  double maximumHeight = pHeader->MaximumHeight;

  double minX = std::numeric_limits<double>::max();
  double minY = std::numeric_limits<double>::max();
  double minZ = std::numeric_limits<double>::max();
  double maxX = std::numeric_limits<double>::lowest();
  double maxY = std::numeric_limits<double>::lowest();
  double maxZ = std::numeric_limits<double>::lowest();

  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  const CesiumGeospatial::GlobeRectangle& rectangle = pRegion->getRectangle();
  double west = rectangle.getWest();
  double south = rectangle.getSouth();
  double east = rectangle.getEast();
  double north = rectangle.getNorth();

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

    double longitude = Math::lerp(west, east, uRatio);
    double latitude = Math::lerp(south, north, vRatio);
    double heightMeters = Math::lerp(minimumHeight, maximumHeight, heightRatio);

    glm::dvec3 position = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, heightMeters));
    position -= center;
    outputPositions[positionOutputIndex++] = static_cast<float>(position.x);
    outputPositions[positionOutputIndex++] = static_cast<float>(position.y);
    outputPositions[positionOutputIndex++] = static_cast<float>(position.z);

    minX = glm::min(minX, position.x);
    minY = glm::min(minY, position.y);
    minZ = glm::min(minZ, position.z);

    maxX = glm::max(maxX, position.x);
    maxY = glm::max(maxY, position.y);
    maxZ = glm::max(maxZ, position.z);

    uvsAndHeights.emplace_back(uRatio, vRatio, heightRatio);
  }

  // decode normal vertices of the tile as well as its metadata without skirt
  std::vector<std::byte> outputNormalsBuffer;
  gsl::span<float> outputNormals;
  if (!meshView->octEncodedNormalBuffer.empty()) {
    uint32_t totalNormalFloats = (vertexCount + skirtVertexCount) * 3;
    outputNormalsBuffer.resize(totalNormalFloats * sizeof(float));
    outputNormals = gsl::span<float>(
        reinterpret_cast<float*>(outputNormalsBuffer.data()),
        totalNormalFloats);
    decodeNormals(meshView->octEncodedNormalBuffer, outputNormals);

    outputNormals = gsl::span<float>(
        reinterpret_cast<float*>(outputNormalsBuffer.data()),
        outputNormalsBuffer.size() / sizeof(float));
  }

  // decode metadata
  if (meshView->metadataJsonLength > 0) {
    processMetadata(pLogger, id, meshView->metadataJsonBuffer, *pResult);
  }

  // indices buffer for gltf to include tile and skirt indices. Caution of
  // indices type since adding skirt means the number of vertices is potentially
  // over maximum of uint16_t
  std::vector<std::byte> outputIndicesBuffer;
  uint32_t indexSizeBytes =
      meshView->indexType == QuantizedMeshIndexType::UnsignedInt
          ? sizeof(uint32_t)
          : sizeof(uint16_t);
  double skirtHeight = calculateSkirtHeight(ellipsoid, rectangle);
  double longitudeOffset = (east - west) * 0.0001;
  double latitudeOffset = (north - south) * 0.0001;
  if (meshView->indexType == QuantizedMeshIndexType::UnsignedInt) {
    // decode the tile indices without skirt.
    size_t outputIndicesCount = indicesCount + skirtIndicesCount;
    outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint32_t));
    gsl::span<const uint32_t> indices(
        reinterpret_cast<const uint32_t*>(meshView->indicesBuffer.data()),
        indicesCount);
    gsl::span<uint32_t> outputIndices(
        reinterpret_cast<uint32_t*>(outputIndicesBuffer.data()),
        outputIndicesCount);
    decodeIndices(indices, outputIndices);

    // generate normals if no provided
    if (outputNormalsBuffer.empty()) {
      outputNormalsBuffer =
          generateNormals(outputPositions, outputIndices, indicesCount);
      outputNormals = gsl::span<float>(
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
        outputIndices);

    indexSizeBytes = sizeof(uint32_t);
  } else {
    size_t outputIndicesCount = indicesCount + skirtIndicesCount;
    gsl::span<const uint16_t> indices(
        reinterpret_cast<const uint16_t*>(meshView->indicesBuffer.data()),
        indicesCount);
    if (vertexCount + skirtVertexCount < std::numeric_limits<uint16_t>::max()) {
      // decode the tile indices without skirt.
      outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint16_t));
      gsl::span<uint16_t> outputIndices(
          reinterpret_cast<uint16_t*>(outputIndicesBuffer.data()),
          outputIndicesCount);
      decodeIndices(indices, outputIndices);

      // generate normals if no provided
      if (outputNormalsBuffer.empty()) {
        outputNormalsBuffer =
            generateNormals(outputPositions, outputIndices, indicesCount);
        outputNormals = gsl::span<float>(
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
          outputIndices);

      indexSizeBytes = sizeof(uint16_t);
    } else {
      outputIndicesBuffer.resize(outputIndicesCount * sizeof(uint32_t));
      gsl::span<uint32_t> outputIndices(
          reinterpret_cast<uint32_t*>(outputIndicesBuffer.data()),
          outputIndicesCount);
      decodeIndices(indices, outputIndices);

      // generate normals if no provided
      if (outputNormalsBuffer.empty()) {
        outputNormalsBuffer =
            generateNormals(outputPositions, outputIndices, indicesCount);
        outputNormals = gsl::span<float>(
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
          outputIndices);

      indexSizeBytes = sizeof(uint32_t);
    }
  }

  // create gltf
  pResult->model.emplace();
  CesiumGltf::Model& model = pResult->model.value();

  size_t meshId = model.meshes.size();
  model.meshes.emplace_back();
  CesiumGltf::Mesh& mesh = model.meshes[meshId];
  mesh.primitives.emplace_back();

  CesiumGltf::MeshPrimitive& primitive = mesh.primitives[0];
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
  primitive.material = 0;

  // add position buffer to gltf
  size_t positionBufferId = model.buffers.size();
  model.buffers.emplace_back();
  CesiumGltf::Buffer& positionBuffer = model.buffers[positionBufferId];
  positionBuffer.cesium.data = std::move(outputPositionsBuffer);

  size_t positionBufferViewId = model.bufferViews.size();
  model.bufferViews.emplace_back();
  CesiumGltf::BufferView& positionBufferView =
      model.bufferViews[positionBufferViewId];
  positionBufferView.buffer = int32_t(positionBufferId);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteStride = 3 * sizeof(float);
  positionBufferView.byteLength = int64_t(positionBuffer.cesium.data.size());
  positionBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  size_t positionAccessorId = model.accessors.size();
  model.accessors.emplace_back();
  CesiumGltf::Accessor& positionAccessor = model.accessors[positionAccessorId];
  positionAccessor.bufferView = static_cast<int>(positionBufferViewId);
  positionAccessor.byteOffset = 0;
  positionAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  positionAccessor.count = vertexCount + skirtVertexCount;
  positionAccessor.type = CesiumGltf::Accessor::Type::VEC3;
  positionAccessor.min = {minX, minY, minZ};
  positionAccessor.max = {maxX, maxY, maxZ};

  primitive.attributes.emplace("POSITION", int32_t(positionAccessorId));

  // add normal buffer to gltf if there are any
  if (!outputNormalsBuffer.empty()) {
    size_t normalBufferId = model.buffers.size();
    model.buffers.emplace_back();
    CesiumGltf::Buffer& normalBuffer = model.buffers[normalBufferId];
    normalBuffer.cesium.data = std::move(outputNormalsBuffer);

    size_t normalBufferViewId = model.bufferViews.size();
    model.bufferViews.emplace_back();
    CesiumGltf::BufferView& normalBufferView =
        model.bufferViews[normalBufferViewId];
    normalBufferView.buffer = int32_t(normalBufferId);
    normalBufferView.byteOffset = 0;
    normalBufferView.byteStride = 3 * sizeof(float);
    normalBufferView.byteLength = int64_t(normalBuffer.cesium.data.size());
    normalBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    size_t normalAccessorId = model.accessors.size();
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
  size_t indicesBufferId = model.buffers.size();
  model.buffers.emplace_back();
  CesiumGltf::Buffer& indicesBuffer = model.buffers[indicesBufferId];
  indicesBuffer.cesium.data = std::move(outputIndicesBuffer);

  size_t indicesBufferViewId = model.bufferViews.size();
  model.bufferViews.emplace_back();
  CesiumGltf::BufferView& indicesBufferView =
      model.bufferViews[indicesBufferViewId];
  indicesBufferView.buffer = int32_t(indicesBufferId);
  indicesBufferView.byteOffset = 0;
  indicesBufferView.byteLength = int64_t(indicesBuffer.cesium.data.size());
  indicesBufferView.byteStride = indexSizeBytes;
  indicesBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  size_t indicesAccessorId = model.accessors.size();
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
  skirtMeshMetadata.meshCenter = center;
  skirtMeshMetadata.skirtWestHeight = skirtHeight;
  skirtMeshMetadata.skirtSouthHeight = skirtHeight;
  skirtMeshMetadata.skirtEastHeight = skirtHeight;
  skirtMeshMetadata.skirtNorthHeight = skirtHeight;

  primitive.extras = SkirtMeshMetadata::createGltfExtras(skirtMeshMetadata);

  // create node and update bounding volume
  model.nodes.emplace_back();
  CesiumGltf::Node& node = model.nodes[0];
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

  pResult->updatedBoundingVolume =
      BoundingRegion(rectangle, minimumHeight, maximumHeight);

  if (pResult->model) {
    pResult->model.value().extras["Cesium3DTiles_TileUrl"] = url;
  }

  return pResult;
}

struct TileRange {
  uint32_t minimumX;
  uint32_t minimumY;
  uint32_t maximumX;
  uint32_t maximumY;
};

static void processMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const QuadtreeTileID& tileID,
    gsl::span<const char> metadataString,
    TileContentLoadResult& result) {
  rapidjson::Document metadata;
  metadata.Parse(
      reinterpret_cast<const char*>(metadataString.data()),
      metadataString.size());

  if (metadata.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing metadata, error code {} at byte offset {}",
        metadata.GetParseError(),
        metadata.GetErrorOffset());
    return;
  }

  auto availableIt = metadata.FindMember("available");
  if (availableIt == metadata.MemberEnd() || !availableIt->value.IsArray()) {
    return;
  }

  const auto& available = availableIt->value;
  if (available.Size() == 0) {
    return;
  }

  uint32_t level = tileID.level + 1;
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

      result.availableTileRectangles.push_back(
          CesiumGeometry::QuadtreeTileRectangularRange{
              level,
              JsonHelpers::getUint32OrDefault(rangeJson, "startX", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "startY", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "endX", 0),
              JsonHelpers::getUint32OrDefault(rangeJson, "endY", 0)});
    }

    ++level;
  }
}

} // namespace Cesium3DTiles
