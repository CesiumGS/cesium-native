#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumQuantizedMeshTerrain/QuantizedMeshLoader.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace Cesium3DTilesContent;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumQuantizedMeshTerrain;
using namespace CesiumUtility;

struct QuantizedMeshHeader {
  double centerX;
  double centerY;
  double centerZ;

  float minimumHeight;
  float maximumHeight;

  double boundingSphereCenterX;
  double boundingSphereCenterY;
  double boundingSphereCenterZ;
  double boundingSphereRadius;

  double horizonOcclusionPointX;
  double horizonOcclusionPointY;
  double horizonOcclusionPointZ;
};

template <typename T> struct MeshData {
  std::vector<uint16_t> u;
  std::vector<uint16_t> v;
  std::vector<uint16_t> height;
  std::vector<T> indices;
  std::vector<T> westIndices;
  std::vector<T> southIndices;
  std::vector<T> eastIndices;
  std::vector<T> northIndices;
};

struct Extension {
  uint8_t extensionID;
  std::vector<std::byte> extensionData;
};

template <typename T> struct QuantizedMesh {
  QuantizedMeshHeader header;
  MeshData<T> vertexData;
  std::vector<Extension> extensions;
};

static uint32_t index2DTo1D(uint32_t x, uint32_t y, uint32_t width) {
  return y * width + x;
}

static uint16_t zigzagEncode(int16_t n) {
  return static_cast<uint16_t>((((uint16_t)n << 1) ^ (n >> 15)) & 0xFFFF);
}

static int32_t zigZagDecode(int32_t value) {
  return (value >> 1) ^ (-(value & 1));
}

static void octEncode(glm::vec3 normal, uint8_t& x, uint8_t& y) {
  float inv =
      1.0f / (glm::abs(normal.x) + glm::abs(normal.y) + glm::abs(normal.z));
  glm::vec2 p;
  p.x = normal.x * inv;
  p.y = normal.y * inv;

  if (normal.z <= 0.0) {
    x = static_cast<uint8_t>(Math::toSNorm(
        (1.0f - glm::abs(p.y)) * static_cast<float>(Math::signNotZero(p.x))));
    y = static_cast<uint8_t>(Math::toSNorm(
        (1.0f - glm::abs(p.x)) * static_cast<float>(Math::signNotZero(p.y))));
  } else {
    x = static_cast<uint8_t>(Math::toSNorm(p.x));
    y = static_cast<uint8_t>(Math::toSNorm(p.y));
  }
}

static double calculateSkirtHeight(
    int tileLevel,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const QuadtreeTilingScheme& tilingScheme) {
  static const double terrainHeightmapQuality = 0.25;
  static const uint32_t heightmapWidth = 65;
  double levelZeroMaximumGeometricError =
      ellipsoid.getMaximumRadius() * CesiumUtility::Math::TwoPi *
      terrainHeightmapQuality / (heightmapWidth * tilingScheme.getRootTilesX());

  double levelMaximumGeometricError =
      levelZeroMaximumGeometricError / (1 << tileLevel);
  return levelMaximumGeometricError * 5.0;
}

template <typename T>
static std::vector<std::byte>
convertQuantizedMeshToBinary(const QuantizedMesh<T>& quantizedMesh) {
  // compute the total size of mesh to preallocate
  size_t totalSize = sizeof(quantizedMesh.header) +
                     sizeof(uint32_t) + // vertex data
                     quantizedMesh.vertexData.u.size() * sizeof(uint16_t) +
                     quantizedMesh.vertexData.v.size() * sizeof(uint16_t) +
                     quantizedMesh.vertexData.height.size() * sizeof(uint16_t) +
                     sizeof(uint32_t) + // indices data
                     quantizedMesh.vertexData.indices.size() * sizeof(T) +
                     sizeof(uint32_t) + // west edge
                     quantizedMesh.vertexData.westIndices.size() * sizeof(T) +
                     sizeof(uint32_t) + // south edge
                     quantizedMesh.vertexData.southIndices.size() * sizeof(T) +
                     sizeof(uint32_t) + // east edge
                     quantizedMesh.vertexData.eastIndices.size() * sizeof(T) +
                     sizeof(uint32_t) + // north edge
                     quantizedMesh.vertexData.northIndices.size() * sizeof(T);

  for (const Extension& extension : quantizedMesh.extensions) {
    totalSize +=
        sizeof(uint8_t) + sizeof(uint32_t) + extension.extensionData.size();
  }

  size_t offset = 0;
  size_t length = 0;
  std::vector<std::byte> buffer(totalSize);

  // serialize header
  length = sizeof(quantizedMesh.header);
  std::memcpy(
      buffer.data(),
      &quantizedMesh.header,
      sizeof(quantizedMesh.header));

  // vertex count
  offset += length;
  uint32_t vertexCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
  length = sizeof(vertexCount);
  std::memcpy(buffer.data() + offset, &vertexCount, length);

  // u buffer
  offset += length;
  length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.u.data(),
      length);

  // v buffer
  offset += length;
  length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.v.data(),
      length);

  // height buffer
  offset += length;
  length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.height.data(),
      length);

  // triangle count
  offset += length;
  uint32_t triangleCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.indices.size()) / 3;
  length = sizeof(triangleCount);
  std::memcpy(buffer.data() + offset, &triangleCount, length);

  // indices buffer
  offset += length;
  length = quantizedMesh.vertexData.indices.size() * sizeof(T);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.indices.data(),
      length);

  // west edge count
  offset += length;
  uint32_t westIndicesCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
  length = sizeof(westIndicesCount);
  std::memcpy(buffer.data() + offset, &westIndicesCount, length);

  // west edge buffer
  offset += length;
  length = quantizedMesh.vertexData.westIndices.size() * sizeof(T);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.westIndices.data(),
      length);

  // south edge count
  offset += length;
  uint32_t southIndicesCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.southIndices.size());
  length = sizeof(southIndicesCount);
  std::memcpy(buffer.data() + offset, &southIndicesCount, length);

  // south edge buffer
  offset += length;
  length = quantizedMesh.vertexData.southIndices.size() * sizeof(T);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.southIndices.data(),
      length);

  // east edge count
  offset += length;
  uint32_t eastIndicesCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.eastIndices.size());
  length = sizeof(eastIndicesCount);
  std::memcpy(buffer.data() + offset, &eastIndicesCount, length);

  // east edge buffer
  offset += length;
  length = quantizedMesh.vertexData.eastIndices.size() * sizeof(T);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.eastIndices.data(),
      length);

  // north edge count
  offset += length;
  uint32_t northIndicesCount =
      static_cast<uint32_t>(quantizedMesh.vertexData.northIndices.size());
  length = sizeof(northIndicesCount);
  std::memcpy(buffer.data() + offset, &northIndicesCount, length);

  // north edge buffer
  offset += length;
  length = quantizedMesh.vertexData.northIndices.size() * sizeof(T);
  std::memcpy(
      buffer.data() + offset,
      quantizedMesh.vertexData.northIndices.data(),
      length);

  // serialize extension
  for (const Extension& extension : quantizedMesh.extensions) {
    // serialize extension ID
    offset += length;
    length = sizeof(extension.extensionID);
    std::memcpy(buffer.data() + offset, &extension.extensionID, length);

    // serialize extension length
    offset += length;
    uint32_t extensionLength =
        static_cast<uint32_t>(extension.extensionData.size());
    length = sizeof(extensionLength);
    std::memcpy(buffer.data() + offset, &extensionLength, length);

    // serialize extension data
    offset += length;
    length = extension.extensionData.size();
    std::memcpy(buffer.data() + offset, extension.extensionData.data(), length);
  }

  return buffer;
}

template <class T>
static QuantizedMesh<T> createGridQuantizedMesh(
    const BoundingRegion& region,
    uint32_t width,
    uint32_t height) {
  if (width * height > std::numeric_limits<T>::max()) {
    throw std::invalid_argument("Number of vertices can be overflowed");
  }

  QuantizedMesh<T> quantizedMesh;
  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  Cartographic cartoCenter = region.getRectangle().computeCenter();
  glm::dvec3 center = ellipsoid.cartographicToCartesian(cartoCenter);
  glm::dvec3 corner =
      ellipsoid.cartographicToCartesian(region.getRectangle().getNortheast());

  quantizedMesh.header.centerX = center.x;
  quantizedMesh.header.centerY = center.y;
  quantizedMesh.header.centerZ = center.z;

  quantizedMesh.header.minimumHeight =
      static_cast<float>(region.getMinimumHeight());
  quantizedMesh.header.maximumHeight =
      static_cast<float>(region.getMaximumHeight());

  quantizedMesh.header.boundingSphereCenterX = center.x;
  quantizedMesh.header.boundingSphereCenterY = center.y;
  quantizedMesh.header.boundingSphereCenterZ = center.z;
  quantizedMesh.header.boundingSphereRadius = glm::distance(center, corner);

  quantizedMesh.header.horizonOcclusionPointX = 0.0;
  quantizedMesh.header.horizonOcclusionPointY = 0.0;
  quantizedMesh.header.horizonOcclusionPointZ = 0.0;

  uint16_t lastU = 0;
  uint16_t lastV = 0;

  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      // encode u, v, and height buffers
      uint16_t u = static_cast<uint16_t>(
          (static_cast<double>(x) / static_cast<double>(width - 1)) * 32767.0);
      uint16_t v = static_cast<uint16_t>(
          ((static_cast<double>(y) / static_cast<double>(height - 1))) *
          32767.0);
      int16_t deltaU = static_cast<int16_t>(u - lastU);
      int16_t deltaV = static_cast<int16_t>(v - lastV);
      quantizedMesh.vertexData.u.emplace_back(zigzagEncode(deltaU));
      quantizedMesh.vertexData.v.emplace_back(zigzagEncode(deltaV));
      quantizedMesh.vertexData.height.push_back(0);

      lastU = u;
      lastV = v;

      if (x < width - 1 && y < height - 1) {
        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x, y, width)));
        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x + 1, y, width)));
        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x, y + 1, width)));

        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x + 1, y, width)));
        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x + 1, y + 1, width)));
        quantizedMesh.vertexData.indices.emplace_back(
            static_cast<T>(index2DTo1D(x, y + 1, width)));
      }

      if (y == 0) {
        quantizedMesh.vertexData.southIndices.emplace_back(
            static_cast<T>(index2DTo1D(x, y, width)));
      }

      if (y == height - 1) {
        quantizedMesh.vertexData.northIndices.emplace_back(
            static_cast<T>(index2DTo1D(x, y, width)));
      }

      if (x == 0) {
        quantizedMesh.vertexData.westIndices.emplace_back(
            static_cast<T>(index2DTo1D(x, y, width)));
      }

      if (x == width - 1) {
        quantizedMesh.vertexData.eastIndices.emplace_back(
            static_cast<T>(index2DTo1D(x, y, width)));
      }
    }
  }

  // hight water mark encoding indices
  T hightWatermark = 0;
  for (T& index : quantizedMesh.vertexData.indices) {
    T originalIndex = index;
    index = static_cast<T>(hightWatermark - index);
    if (originalIndex == hightWatermark) {
      ++hightWatermark;
    }
  }

  return quantizedMesh;
}

template <class T, class I>
void checkGridMesh(
    const QuantizedMesh<T>& quantizedMesh,
    const AccessorView<I>& indices,
    const AccessorView<glm::vec3>& positions,
    const QuadtreeTilingScheme& tilingScheme,
    const Ellipsoid& ellipsoid,
    const CesiumGeometry::Rectangle& tileRectangle,
    uint32_t verticesWidth,
    uint32_t verticesHeight) {
  double west = tileRectangle.minimumX;
  double south = tileRectangle.minimumY;
  double east = tileRectangle.maximumX;
  double north = tileRectangle.maximumY;

  // check grid mesh without skirt
  const std::vector<uint16_t>& uBuffer = quantizedMesh.vertexData.u;
  const std::vector<uint16_t>& vBuffer = quantizedMesh.vertexData.v;
  int32_t u = 0;
  int32_t v = 0;

  std::vector<glm::dvec2> uvs;
  uvs.reserve(static_cast<size_t>(verticesWidth * verticesHeight));
  uint32_t positionIdx = 0;
  uint32_t idx = 0;
  for (uint32_t y = 0; y < verticesHeight; ++y) {
    for (uint32_t x = 0; x < verticesWidth; ++x) {
      u += zigZagDecode(uBuffer[positionIdx]);
      v += zigZagDecode(vBuffer[positionIdx]);

      // check that u ratio and v ratio is similar to grid ratio
      double uRatio = static_cast<double>(u) / 32767.0;
      double vRatio = static_cast<double>(v) / 32767.0;
      REQUIRE(Math::equalsEpsilon(
          uRatio,
          static_cast<double>(x) / static_cast<double>(verticesWidth - 1),
          Math::Epsilon4));
      REQUIRE(Math::equalsEpsilon(
          vRatio,
          static_cast<double>(y) / static_cast<double>(verticesHeight - 1),
          Math::Epsilon4));

      // check grid positions
      double longitude = Math::lerp(west, east, uRatio);
      double latitude = Math::lerp(south, north, vRatio);
      glm::dvec3 expectPosition =
          ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude));

      glm::dvec3 position = static_cast<glm::dvec3>(positions[positionIdx]);
      position += glm::dvec3(
          quantizedMesh.header.boundingSphereCenterX,
          quantizedMesh.header.boundingSphereCenterY,
          quantizedMesh.header.boundingSphereCenterZ);

      REQUIRE(
          Math::equalsEpsilon(position.x, expectPosition.x, Math::Epsilon3));
      REQUIRE(
          Math::equalsEpsilon(position.y, expectPosition.y, Math::Epsilon3));
      REQUIRE(
          Math::equalsEpsilon(position.z, expectPosition.z, Math::Epsilon3));
      ++positionIdx;

      // check indices
      if (x < verticesWidth - 1 && y < verticesHeight - 1) {
        REQUIRE(
            indices[idx++] == static_cast<I>(index2DTo1D(x, y, verticesWidth)));
        REQUIRE(
            indices[idx++] ==
            static_cast<I>(index2DTo1D(x + 1, y, verticesWidth)));
        REQUIRE(
            indices[idx++] ==
            static_cast<I>(index2DTo1D(x, y + 1, verticesWidth)));

        REQUIRE(
            indices[idx++] ==
            static_cast<I>(index2DTo1D(x + 1, y, verticesWidth)));
        REQUIRE(
            indices[idx++] ==
            static_cast<I>(index2DTo1D(x + 1, y + 1, verticesWidth)));
        REQUIRE(
            indices[idx++] ==
            static_cast<I>(index2DTo1D(x, y + 1, verticesWidth)));
      }

      uvs.emplace_back(uRatio, vRatio);
    }
  }

  // make sure there are skirts in there
  size_t westIndicesCount = quantizedMesh.vertexData.westIndices.size();
  size_t southIndicesCount = quantizedMesh.vertexData.southIndices.size();
  size_t eastIndicesCount = quantizedMesh.vertexData.eastIndices.size();
  size_t northIndicesCount = quantizedMesh.vertexData.northIndices.size();

  size_t gridVerticesCount =
      static_cast<size_t>(verticesWidth * verticesHeight);
  size_t gridIndicesCount =
      static_cast<size_t>((verticesHeight - 1) * (verticesWidth - 1) * 6);
  size_t totalSkirtVertices = westIndicesCount + southIndicesCount +
                              eastIndicesCount + northIndicesCount;
  size_t totalSkirtIndices = (totalSkirtVertices - 4) * 6;

  double skirtHeight = calculateSkirtHeight(10, ellipsoid, tilingScheme);
  double longitudeOffset = (west - east) * 0.0001;
  double latitudeOffset = (north - south) * 0.0001;

  REQUIRE(totalSkirtIndices == size_t(indices.size()) - gridIndicesCount);
  REQUIRE(totalSkirtVertices == size_t(positions.size()) - gridVerticesCount);

  size_t currentVertexCount = gridVerticesCount;
  for (size_t i = 0; i < westIndicesCount; ++i) {
    T westIndex = quantizedMesh.vertexData.westIndices[i];
    double longitude = west + longitudeOffset;
    double latitude = Math::lerp(south, north, uvs[westIndex].y);
    glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, -skirtHeight));

    glm::dvec3 position =
        static_cast<glm::dvec3>(positions[int64_t(currentVertexCount + i)]);
    position += glm::dvec3(
        quantizedMesh.header.boundingSphereCenterX,
        quantizedMesh.header.boundingSphereCenterY,
        quantizedMesh.header.boundingSphereCenterZ);
    REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::Epsilon3));
  }

  currentVertexCount += westIndicesCount;
  for (size_t i = 0; i < southIndicesCount; ++i) {
    T southIndex =
        quantizedMesh.vertexData.southIndices[southIndicesCount - 1 - i];
    double longitude = Math::lerp(west, east, uvs[southIndex].x);
    double latitude = south - latitudeOffset;
    glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, -skirtHeight));

    glm::dvec3 position =
        static_cast<glm::dvec3>(positions[int64_t(currentVertexCount + i)]);
    position += glm::dvec3(
        quantizedMesh.header.boundingSphereCenterX,
        quantizedMesh.header.boundingSphereCenterY,
        quantizedMesh.header.boundingSphereCenterZ);
    REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::Epsilon3));
  }

  currentVertexCount += southIndicesCount;
  for (size_t i = 0; i < eastIndicesCount; ++i) {
    T eastIndex =
        quantizedMesh.vertexData.eastIndices[eastIndicesCount - 1 - i];
    double longitude = east + longitudeOffset;
    double latitude = Math::lerp(south, north, uvs[eastIndex].y);
    glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, -skirtHeight));

    glm::dvec3 position =
        static_cast<glm::dvec3>(positions[int64_t(currentVertexCount + i)]);
    position += glm::dvec3(
        quantizedMesh.header.boundingSphereCenterX,
        quantizedMesh.header.boundingSphereCenterY,
        quantizedMesh.header.boundingSphereCenterZ);
    REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::Epsilon2));
    REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::Epsilon3));
  }

  currentVertexCount += eastIndicesCount;
  for (size_t i = 0; i < northIndicesCount; ++i) {
    T northIndex = quantizedMesh.vertexData.northIndices[i];
    double longitude = Math::lerp(west, east, uvs[northIndex].x);
    double latitude = north + latitudeOffset;
    glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(
        Cartographic(longitude, latitude, -skirtHeight));

    glm::dvec3 position =
        static_cast<glm::dvec3>(positions[int64_t(currentVertexCount + i)]);
    position += glm::dvec3(
        quantizedMesh.header.boundingSphereCenterX,
        quantizedMesh.header.boundingSphereCenterY,
        quantizedMesh.header.boundingSphereCenterZ);
    REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::Epsilon3));
    REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::Epsilon3));
  }
}

template <class T, class I>
static void checkGeneratedGridNormal(
    const QuantizedMesh<T>& quantizedMesh,
    const AccessorView<glm::vec3>& normals,
    const AccessorView<glm::vec3>& positions,
    const AccessorView<I>& indices,
    const glm::vec3& geodeticNormal,
    uint32_t verticesWidth,
    uint32_t verticesHeight) {
  uint32_t totalGridIndices = (verticesWidth - 1) * (verticesHeight - 1) * 6;
  std::vector<glm::vec3> expectedNormals(
      static_cast<size_t>(verticesWidth * verticesHeight));
  for (uint32_t i = 0; i < totalGridIndices; i += 3) {
    I id0 = indices[i];
    I id1 = indices[i + 1];
    I id2 = indices[i + 2];

    glm::vec3 p0 = positions[id0];
    glm::vec3 p1 = positions[id1];
    glm::vec3 p2 = positions[id2];

    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    expectedNormals[id0] += normal;
    expectedNormals[id1] += normal;
    expectedNormals[id2] += normal;
  }

  for (size_t i = 0; i < expectedNormals.size(); ++i) {
    glm::vec3& expectedNormal = expectedNormals[i];
    glm::vec3 normal = normals[int64_t(i)];

    if (!Math::equalsEpsilon(
            glm::dot(expectedNormals[i], expectedNormals[i]),
            0.0,
            Math::Epsilon7)) {
      expectedNormal = glm::normalize(expectedNormals[i]);

      // make sure normal points to the direction of geodetic normal for grid
      // only
      REQUIRE(glm::dot(normal, geodeticNormal) >= 0.0);

      REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
      REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
      REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));
    } else {
      REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
      REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
      REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));
    }
  }

  // make sure there are skirts in there
  size_t westIndicesCount = quantizedMesh.vertexData.westIndices.size();
  size_t southIndicesCount = quantizedMesh.vertexData.southIndices.size();
  size_t eastIndicesCount = quantizedMesh.vertexData.eastIndices.size();
  size_t northIndicesCount = quantizedMesh.vertexData.northIndices.size();

  size_t gridVerticesCount =
      static_cast<size_t>(verticesWidth * verticesHeight);
  size_t totalSkirtVertices = westIndicesCount + southIndicesCount +
                              eastIndicesCount + northIndicesCount;

  REQUIRE(totalSkirtVertices == size_t(normals.size()) - gridVerticesCount);

  size_t currentVertexCount = gridVerticesCount;
  uint32_t x = 0;
  uint32_t y = 0;
  for (size_t i = 0; i < westIndicesCount; ++i) {
    glm::vec3 normal = normals[int64_t(currentVertexCount + i)];
    glm::vec3 expectedNormal =
        expectedNormals[index2DTo1D(x, y, verticesWidth)];
    REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));

    ++y;
  }

  currentVertexCount += westIndicesCount;
  x = verticesWidth - 1;
  y = 0;
  for (size_t i = 0; i < southIndicesCount; ++i) {
    glm::vec3 normal = normals[int64_t(currentVertexCount + i)];
    glm::vec3 expectedNormal =
        expectedNormals[index2DTo1D(x, y, verticesWidth)];
    REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));

    --x;
  }

  currentVertexCount += southIndicesCount;
  x = verticesWidth - 1;
  y = verticesHeight - 1;
  for (size_t i = 0; i < eastIndicesCount; ++i) {
    glm::vec3 normal = normals[int64_t(currentVertexCount + i)];
    glm::vec3 expectedNormal =
        expectedNormals[index2DTo1D(x, y, verticesWidth)];
    REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));

    --y;
  }

  currentVertexCount += eastIndicesCount;
  x = 0;
  y = verticesHeight - 1;
  for (size_t i = 0; i < northIndicesCount; ++i) {
    glm::vec3 normal = normals[int64_t(currentVertexCount + i)];
    glm::vec3 expectedNormal =
        expectedNormals[index2DTo1D(x, y, verticesWidth)];
    REQUIRE(Math::equalsEpsilon(normal.x, expectedNormal.x, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.y, expectedNormal.y, Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(normal.z, expectedNormal.z, Math::Epsilon7));

    ++x;
  }
}

static void checkGltfSanity(const Model& model) {
  CHECK(model.asset.version == "2.0");
  REQUIRE(model.scene >= 0);
  REQUIRE(size_t(model.scene) < model.scenes.size());
  CHECK(!model.getSafe(model.scenes, model.scene).nodes.empty());

  for (const Buffer& buffer : model.buffers) {
    CHECK(buffer.byteLength > 0);
    CHECK(buffer.byteLength == static_cast<int64_t>(buffer.cesium.data.size()));
  }

  for (const BufferView& bufferView : model.bufferViews) {
    CHECK(bufferView.byteLength > 0);
  }

  for (const Mesh& mesh : model.meshes) {
    for (const MeshPrimitive& primitive : mesh.primitives) {
      const Accessor* pIndicesAccessor =
          model.getSafe(&model.accessors, primitive.indices);
      REQUIRE(pIndicesAccessor);
      const BufferView* pIndicesBufferView =
          model.getSafe(&model.bufferViews, pIndicesAccessor->bufferView);
      REQUIRE(pIndicesBufferView);

      CHECK(
          pIndicesBufferView->target ==
          BufferView::Target::ELEMENT_ARRAY_BUFFER);

      for (const auto& attribute : primitive.attributes) {
        const Accessor* pAttributeAccessor =
            model.getSafe(&model.accessors, attribute.second);
        REQUIRE(pAttributeAccessor);
        const BufferView* pAttributeBufferView =
            model.getSafe(&model.bufferViews, pAttributeAccessor->bufferView);
        REQUIRE(pAttributeBufferView);

        CHECK(pAttributeBufferView->target == BufferView::Target::ARRAY_BUFFER);

        std::vector<double> min = pAttributeAccessor->min;
        std::vector<double> max = pAttributeAccessor->max;
        CHECK(min.size() == max.size());

        if (!min.empty()) {
          createAccessorView(
              model,
              *pAttributeAccessor,
              [min, max](const auto& accessorView) {
                using ElementType = decltype(accessorView[0].value[0]);
                for (int64_t i = 0; i < accessorView.size(); ++i) {
                  auto v = accessorView[i];
                  REQUIRE(sizeof(v.value) / sizeof(ElementType) == min.size());
                  REQUIRE(sizeof(v.value) / sizeof(ElementType) == max.size());
                  for (size_t j = 0; j < min.size(); ++j) {
                    CHECK(v.value[j] >= ElementType(min[j]));
                    CHECK(v.value[j] <= ElementType(max[j]));
                  }
                }
              });
        }
      }
    }
  }
}

TEST_CASE("Test converting quantized mesh to gltf with skirt") {
  registerAllTileContentTypes();

  // mock context
  Ellipsoid ellipsoid = Ellipsoid::WGS84;
  CesiumGeometry::Rectangle rectangle(
      glm::radians(-180.0),
      glm::radians(-90.0),
      glm::radians(180.0),
      glm::radians(90.0));
  QuadtreeTilingScheme tilingScheme(rectangle, 2, 1);

  SUBCASE("Check quantized mesh that has uint16_t indices") {
    // mock quantized mesh
    uint32_t verticesWidth = 3;
    uint32_t verticesHeight = 3;
    QuadtreeTileID tileID(10, 0, 0);
    CesiumGeometry::Rectangle tileRectangle =
        tilingScheme.tileToRectangle(tileID);
    BoundingRegion boundingVolume = BoundingRegion(
        GlobeRectangle(
            tileRectangle.minimumX,
            tileRectangle.minimumY,
            tileRectangle.maximumX,
            tileRectangle.maximumY),
        0.0,
        0.0,
        Ellipsoid::WGS84);
    QuantizedMesh<uint16_t> quantizedMesh = createGridQuantizedMesh<uint16_t>(
        boundingVolume,
        verticesWidth,
        verticesHeight);

    // convert to gltf
    std::vector<std::byte> quantizedMeshBin =
        convertQuantizedMeshToBinary(quantizedMesh);
    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    QuantizedMeshLoadResult loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);
    REQUIRE(!loadResult.errors.hasErrors());
    REQUIRE(loadResult.model != std::nullopt);

    checkGltfSanity(*loadResult.model);

    // make sure the gltf is the grid
    const CesiumGltf::Model& model = *loadResult.model;
    const CesiumGltf::Mesh& mesh = model.meshes.front();
    const CesiumGltf::MeshPrimitive& primitive = mesh.primitives.front();

    // make sure mesh contains grid mesh and skirts at the end
    AccessorView<uint16_t> indices(model, primitive.indices);
    CHECK(indices.status() == AccessorViewStatus::Valid);
    AccessorView<glm::vec3> positions(
        model,
        primitive.attributes.at("POSITION"));
    CHECK(positions.status() == AccessorViewStatus::Valid);

    checkGridMesh(
        quantizedMesh,
        indices,
        positions,
        tilingScheme,
        ellipsoid,
        tileRectangle,
        verticesWidth,
        verticesHeight);

    // check normal
    AccessorView<glm::vec3> normals(model, primitive.attributes.at("NORMAL"));
    CHECK(normals.status() == AccessorViewStatus::Valid);

    Cartographic center = boundingVolume.getRectangle().computeCenter();
    glm::vec3 geodeticNormal =
        static_cast<glm::vec3>(ellipsoid.geodeticSurfaceNormal(center));
    checkGeneratedGridNormal(
        quantizedMesh,
        normals,
        positions,
        indices,
        geodeticNormal,
        verticesWidth,
        verticesHeight);
  }

  SUBCASE("Check quantized mesh that has uint32_t indices") {
    // mock quantized mesh
    uint32_t verticesWidth = 300;
    uint32_t verticesHeight = 300;
    QuadtreeTileID tileID(10, 0, 0);
    CesiumGeometry::Rectangle tileRectangle =
        tilingScheme.tileToRectangle(tileID);
    BoundingRegion boundingVolume = BoundingRegion(
        GlobeRectangle(
            tileRectangle.minimumX,
            tileRectangle.minimumY,
            tileRectangle.maximumX,
            tileRectangle.maximumY),
        0.0,
        0.0,
        Ellipsoid::WGS84);
    QuantizedMesh<uint32_t> quantizedMesh = createGridQuantizedMesh<uint32_t>(
        boundingVolume,
        verticesWidth,
        verticesHeight);

    // convert to gltf
    std::vector<std::byte> quantizedMeshBin =
        convertQuantizedMeshToBinary(quantizedMesh);
    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);
    REQUIRE(!loadResult.errors.hasErrors());
    REQUIRE(loadResult.model != std::nullopt);

    checkGltfSanity(*loadResult.model);

    // make sure the gltf is the grid
    const CesiumGltf::Model& model = *loadResult.model;
    const CesiumGltf::Mesh& mesh = model.meshes.front();
    const CesiumGltf::MeshPrimitive& primitive = mesh.primitives.front();

    // make sure mesh contains grid mesh and skirts at the end
    AccessorView<uint32_t> indices(model, primitive.indices);
    CHECK(indices.status() == AccessorViewStatus::Valid);
    AccessorView<glm::vec3> positions(
        model,
        primitive.attributes.at("POSITION"));
    CHECK(positions.status() == AccessorViewStatus::Valid);

    checkGridMesh(
        quantizedMesh,
        indices,
        positions,
        tilingScheme,
        ellipsoid,
        tileRectangle,
        verticesWidth,
        verticesHeight);

    // check normal
    AccessorView<glm::vec3> normals(model, primitive.attributes.at("NORMAL"));
    CHECK(normals.status() == AccessorViewStatus::Valid);

    Cartographic center = boundingVolume.getRectangle().computeCenter();
    glm::vec3 geodeticNormal =
        static_cast<glm::vec3>(ellipsoid.geodeticSurfaceNormal(center));
    checkGeneratedGridNormal(
        quantizedMesh,
        normals,
        positions,
        indices,
        geodeticNormal,
        verticesWidth,
        verticesHeight);
  }

  SUBCASE("Check 16 bit indices turns to 32 bits when adding skirt") {
    // mock quantized mesh
    uint32_t verticesWidth = 255;
    uint32_t verticesHeight = 255;
    QuadtreeTileID tileID(10, 0, 0);
    CesiumGeometry::Rectangle tileRectangle =
        tilingScheme.tileToRectangle(tileID);
    BoundingRegion boundingVolume = BoundingRegion(
        GlobeRectangle(
            tileRectangle.minimumX,
            tileRectangle.minimumY,
            tileRectangle.maximumX,
            tileRectangle.maximumY),
        0.0,
        0.0,
        Ellipsoid::WGS84);
    QuantizedMesh<uint16_t> quantizedMesh = createGridQuantizedMesh<uint16_t>(
        boundingVolume,
        verticesWidth,
        verticesHeight);

    // convert to gltf
    std::vector<std::byte> quantizedMeshBin =
        convertQuantizedMeshToBinary(quantizedMesh);
    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);
    REQUIRE(!loadResult.errors.hasErrors());
    REQUIRE(loadResult.model != std::nullopt);

    checkGltfSanity(*loadResult.model);

    // make sure the gltf is the grid
    const CesiumGltf::Model& model = *loadResult.model;
    const CesiumGltf::Mesh& mesh = model.meshes.front();
    const CesiumGltf::MeshPrimitive& primitive = mesh.primitives.front();

    // make sure mesh contains grid mesh and skirts at the end
    AccessorView<uint32_t> indices(model, primitive.indices);
    CHECK(indices.status() == AccessorViewStatus::Valid);
    AccessorView<glm::vec3> positions(
        model,
        primitive.attributes.at("POSITION"));
    CHECK(positions.status() == AccessorViewStatus::Valid);

    checkGridMesh(
        quantizedMesh,
        indices,
        positions,
        tilingScheme,
        ellipsoid,
        tileRectangle,
        verticesWidth,
        verticesHeight);

    // check normal
    AccessorView<glm::vec3> normals(model, primitive.attributes.at("NORMAL"));
    CHECK(normals.status() == AccessorViewStatus::Valid);

    Cartographic center = boundingVolume.getRectangle().computeCenter();
    glm::vec3 geodeticNormal =
        static_cast<glm::vec3>(ellipsoid.geodeticSurfaceNormal(center));
    checkGeneratedGridNormal(
        quantizedMesh,
        normals,
        positions,
        indices,
        geodeticNormal,
        verticesWidth,
        verticesHeight);
  }

  SUBCASE("Check quantized mesh that has oct normal") {
    // mock quantized mesh
    uint32_t verticesWidth = 3;
    uint32_t verticesHeight = 3;
    QuadtreeTileID tileID(10, 0, 0);
    CesiumGeometry::Rectangle tileRectangle =
        tilingScheme.tileToRectangle(tileID);
    BoundingRegion boundingVolume = BoundingRegion(
        GlobeRectangle(
            tileRectangle.minimumX,
            tileRectangle.minimumY,
            tileRectangle.maximumX,
            tileRectangle.maximumY),
        0.0,
        0.0,
        Ellipsoid::WGS84);
    QuantizedMesh<uint16_t> quantizedMesh = createGridQuantizedMesh<uint16_t>(
        boundingVolume,
        verticesWidth,
        verticesHeight);

    // add oct-encoded normal extension. This is just a random direction and
    // not really normal. We want to make sure the normal is written to the gltf
    glm::vec3 normal = glm::normalize(glm::vec3(0.2, 1.4, 0.3));
    uint8_t x = 0, y = 0;
    octEncode(normal, x, y);
    std::vector<std::byte> octNormals(
        static_cast<size_t>(verticesWidth * verticesHeight * 2));
    for (size_t i = 0; i < octNormals.size(); i += 2) {
      octNormals[i] = std::byte(x);
      octNormals[i + 1] = std::byte(y);
    }

    Extension octNormalExtension;
    octNormalExtension.extensionID = 1;
    octNormalExtension.extensionData = std::move(octNormals);

    quantizedMesh.extensions.emplace_back(std::move(octNormalExtension));

    // convert to gltf
    std::vector<std::byte> quantizedMeshBin =
        convertQuantizedMeshToBinary(quantizedMesh);
    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);
    REQUIRE(!loadResult.errors.hasErrors());
    REQUIRE(loadResult.model != std::nullopt);

    checkGltfSanity(*loadResult.model);

    // make sure the gltf has normals
    const CesiumGltf::Model& model = *loadResult.model;
    const CesiumGltf::Mesh& mesh = model.meshes.front();
    const CesiumGltf::MeshPrimitive& primitive = mesh.primitives.front();

    size_t westIndicesCount = quantizedMesh.vertexData.westIndices.size();
    size_t southIndicesCount = quantizedMesh.vertexData.southIndices.size();
    size_t eastIndicesCount = quantizedMesh.vertexData.eastIndices.size();
    size_t northIndicesCount = quantizedMesh.vertexData.northIndices.size();
    size_t totalSkirtVerticesCount = westIndicesCount + southIndicesCount +
                                     eastIndicesCount + northIndicesCount;

    AccessorView<glm::vec3> normals(model, primitive.attributes.at("NORMAL"));
    CHECK(normals.status() == AccessorViewStatus::Valid);

    REQUIRE(
        static_cast<size_t>(normals.size()) ==
        (static_cast<size_t>(verticesWidth * verticesHeight) +
         totalSkirtVerticesCount));
    for (int64_t i = 0; i < normals.size(); ++i) {
      REQUIRE(Math::equalsEpsilon(normals[i].x, normal.x, Math::Epsilon2));
      REQUIRE(Math::equalsEpsilon(normals[i].y, normal.y, Math::Epsilon2));
      REQUIRE(Math::equalsEpsilon(normals[i].z, normal.z, Math::Epsilon2));
    }
  }
}

TEST_CASE("Test converting ill-formed quantized mesh") {
  registerAllTileContentTypes();

  // mock context
  CesiumGeometry::Rectangle rectangle(
      glm::radians(-180.0),
      glm::radians(-90.0),
      glm::radians(180.0),
      glm::radians(90.0));
  QuadtreeTilingScheme tilingScheme(rectangle, 2, 1);

  // mock quantized mesh
  uint32_t verticesWidth = 3;
  uint32_t verticesHeight = 3;
  QuadtreeTileID tileID(10, 0, 0);
  CesiumGeometry::Rectangle tileRectangle =
      tilingScheme.tileToRectangle(tileID);
  BoundingRegion boundingVolume = BoundingRegion(
      GlobeRectangle(
          tileRectangle.minimumX,
          tileRectangle.minimumY,
          tileRectangle.maximumX,
          tileRectangle.maximumY),
      0.0,
      0.0,
      Ellipsoid::WGS84);
  QuantizedMesh<uint16_t> quantizedMesh = createGridQuantizedMesh<uint16_t>(
      boundingVolume,
      verticesWidth,
      verticesHeight);

  // add oct-encoded normal extension. This is just a random direction and not
  // really normal. We want to make sure the normal is written to the gltf
  glm::vec3 normal = glm::normalize(glm::vec3(0.2, 1.4, 0.3));
  uint8_t x = 0, y = 0;
  octEncode(normal, x, y);
  std::vector<std::byte> octNormals(
      static_cast<size_t>(verticesWidth * verticesHeight * 2));
  for (size_t i = 0; i < octNormals.size(); i += 2) {
    octNormals[i] = std::byte(x);
    octNormals[i + 1] = std::byte(y);
  }

  Extension octNormalExtension;
  octNormalExtension.extensionID = 1;
  octNormalExtension.extensionData = std::move(octNormals);

  quantizedMesh.extensions.emplace_back(std::move(octNormalExtension));

  SUBCASE("Quantized mesh with ill-formed header") {
    std::vector<std::byte> quantizedMeshBin(32);
    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);
    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed vertex data") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                       // header
        sizeof(uint32_t) +                                   // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) // u buffer
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed indices") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                         // header
        sizeof(uint32_t) +                                     // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) + // u buffer
        quantizedMesh.vertexData.v.size() * sizeof(uint16_t) + // v buffer
        quantizedMesh.vertexData.height.size() *
            sizeof(uint16_t) + // height buffer
        sizeof(uint32_t)       // triangle count
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    // serialize v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.v.data(),
        length);

    // serialize height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.height.data(),
        length);

    // serialize triangle count
    offset += length;
    uint32_t triangleCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.indices.size() / 3);
    length = sizeof(triangleCount);
    std::memcpy(quantizedMeshBin.data() + offset, &triangleCount, length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed west edge indices") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                         // header
        sizeof(uint32_t) +                                     // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) + // u buffer
        quantizedMesh.vertexData.v.size() * sizeof(uint16_t) + // v buffer
        quantizedMesh.vertexData.height.size() *
            sizeof(uint16_t) + // height buffer
        sizeof(uint32_t) +     // triangle count
        quantizedMesh.vertexData.indices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) // west edge
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    // serialize v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.v.data(),
        length);

    // serialize height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.height.data(),
        length);

    // serialize triangle count
    offset += length;
    uint32_t triangleCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.indices.size() / 3);
    length = sizeof(triangleCount);
    std::memcpy(quantizedMeshBin.data() + offset, &triangleCount, length);

    // serialize indices
    offset += length;
    length = quantizedMesh.vertexData.indices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.indices.data(),
        length);

    // serialize west edge
    offset += length;
    uint32_t westCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &westCount, length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed south edge indices") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                         // header
        sizeof(uint32_t) +                                     // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) + // u buffer
        quantizedMesh.vertexData.v.size() * sizeof(uint16_t) + // v buffer
        quantizedMesh.vertexData.height.size() *
            sizeof(uint16_t) + // height buffer
        sizeof(uint32_t) +     // triangle count
        quantizedMesh.vertexData.indices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // west edge
        quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) // south edge
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    // serialize v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.v.data(),
        length);

    // serialize height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.height.data(),
        length);

    // serialize triangle count
    offset += length;
    uint32_t triangleCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.indices.size() / 3);
    length = sizeof(triangleCount);
    std::memcpy(quantizedMeshBin.data() + offset, &triangleCount, length);

    // serialize indices
    offset += length;
    length = quantizedMesh.vertexData.indices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.indices.data(),
        length);

    // serialize west edge
    offset += length;
    uint32_t westCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &westCount, length);

    offset += length;
    length = quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.westIndices.data(),
        length);

    // serialize south edge
    offset += length;
    uint32_t southCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.southIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &southCount, length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed east edge indices") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                         // header
        sizeof(uint32_t) +                                     // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) + // u buffer
        quantizedMesh.vertexData.v.size() * sizeof(uint16_t) + // v buffer
        quantizedMesh.vertexData.height.size() *
            sizeof(uint16_t) + // height buffer
        sizeof(uint32_t) +     // triangle count
        quantizedMesh.vertexData.indices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // west edge
        quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // south edge
        quantizedMesh.vertexData.southIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) // east edge
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    // serialize v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.v.data(),
        length);

    // serialize height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.height.data(),
        length);

    // serialize triangle count
    offset += length;
    uint32_t triangleCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.indices.size() / 3);
    length = sizeof(triangleCount);
    std::memcpy(quantizedMeshBin.data() + offset, &triangleCount, length);

    // serialize indices
    offset += length;
    length = quantizedMesh.vertexData.indices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.indices.data(),
        length);

    // serialize west edge
    offset += length;
    uint32_t westCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &westCount, length);

    offset += length;
    length = quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.westIndices.data(),
        length);

    // serialize south edge
    offset += length;
    uint32_t southCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.southIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &southCount, length);

    offset += length;
    length = quantizedMesh.vertexData.southIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.southIndices.data(),
        length);

    // serialize east edge
    offset += length;
    uint32_t eastCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.eastIndices.size());
    length = sizeof(eastCount);
    std::memcpy(quantizedMeshBin.data() + offset, &eastCount, length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }

  SUBCASE("Quantized mesh with ill-formed north edge indices") {
    std::vector<std::byte> quantizedMeshBin(
        sizeof(quantizedMesh.header) +                         // header
        sizeof(uint32_t) +                                     // vertex count
        quantizedMesh.vertexData.u.size() * sizeof(uint16_t) + // u buffer
        quantizedMesh.vertexData.v.size() * sizeof(uint16_t) + // v buffer
        quantizedMesh.vertexData.height.size() *
            sizeof(uint16_t) + // height buffer
        sizeof(uint32_t) +     // triangle count
        quantizedMesh.vertexData.indices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // west edge
        quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // south edge
        quantizedMesh.vertexData.southIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) + // east edge
        quantizedMesh.vertexData.eastIndices.size() * sizeof(uint16_t) +
        sizeof(uint32_t) // north edge
    );

    // serialize header
    size_t offset = 0;
    size_t length = sizeof(quantizedMesh.header);
    std::memcpy(quantizedMeshBin.data(), &quantizedMesh.header, length);

    // serialize vertex count
    offset += length;
    uint32_t vertexCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(quantizedMeshBin.data() + offset, &vertexCount, length);

    // serialize u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.u.data(),
        length);

    // serialize v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.v.data(),
        length);

    // serialize height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.height.data(),
        length);

    // serialize triangle count
    offset += length;
    uint32_t triangleCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.indices.size() / 3);
    length = sizeof(triangleCount);
    std::memcpy(quantizedMeshBin.data() + offset, &triangleCount, length);

    // serialize indices
    offset += length;
    length = quantizedMesh.vertexData.indices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.indices.data(),
        length);

    // serialize west edge
    offset += length;
    uint32_t westCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &westCount, length);

    offset += length;
    length = quantizedMesh.vertexData.westIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.westIndices.data(),
        length);

    // serialize south edge
    offset += length;
    uint32_t southCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.southIndices.size());
    length = sizeof(westCount);
    std::memcpy(quantizedMeshBin.data() + offset, &southCount, length);

    offset += length;
    length = quantizedMesh.vertexData.southIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.southIndices.data(),
        length);

    // serialize east edge
    offset += length;
    uint32_t eastCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.eastIndices.size());
    length = sizeof(eastCount);
    std::memcpy(quantizedMeshBin.data() + offset, &eastCount, length);

    offset += length;
    length = quantizedMesh.vertexData.eastIndices.size() * sizeof(uint16_t);
    std::memcpy(
        quantizedMeshBin.data() + offset,
        quantizedMesh.vertexData.eastIndices.data(),
        length);

    // serialize north edge
    offset += length;
    uint32_t northCount =
        static_cast<uint32_t>(quantizedMesh.vertexData.northIndices.size());
    length = sizeof(northCount);
    std::memcpy(quantizedMeshBin.data() + offset, &northCount, length);

    std::span<const std::byte> data(
        quantizedMeshBin.data(),
        quantizedMeshBin.size());
    auto loadResult =
        QuantizedMeshLoader::load(tileID, boundingVolume, "url", data, false);

    REQUIRE(loadResult.model == std::nullopt);
  }
}
