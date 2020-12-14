#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "CesiumUtility/Math.h"
#include "Cesium3DTiles/GltfAccessor.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeometry/Rectangle.h"

using namespace Cesium3DTiles;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
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

template<typename T>
struct MeshData {
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
    std::vector<uint8_t> extensionData;
};

template<typename T>
struct QuantizedMesh {
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

static double calculateSkirtHeight(int tileLevel, const CesiumGeospatial::Ellipsoid &ellipsoid, const QuadtreeTilingScheme &tilingScheme) {
    static const double terrainHeightmapQuality = 0.25;
    static const uint32_t heightmapWidth = 65;
    double levelZeroMaximumGeometricError = ellipsoid.getMaximumRadius() * CesiumUtility::Math::TWO_PI * terrainHeightmapQuality
        / (heightmapWidth * tilingScheme.getRootTilesX());

    double levelMaximumGeometricError = levelZeroMaximumGeometricError / (1 << tileLevel);
    return levelMaximumGeometricError * 5.0;
}

template<typename T>
static std::vector<uint8_t> convertQuantizedMeshToBinary(const QuantizedMesh<T> &quantizedMesh) {
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
        totalSize += sizeof(uint8_t) + sizeof(uint32_t) + extension.extensionData.size();
    }

    size_t offset = 0;
    size_t length = 0;
    std::vector<uint8_t> buffer(totalSize);

    // serialize header
    length = sizeof(quantizedMesh.header);
    std::memcpy(buffer.data(), &quantizedMesh.header, sizeof(quantizedMesh.header));

    // vertex count
    offset += length;
    uint32_t vertexCount = static_cast<uint32_t>(quantizedMesh.vertexData.u.size());
    length = sizeof(vertexCount);
    std::memcpy(buffer.data() + offset, &vertexCount, length);

    // u buffer
    offset += length;
    length = quantizedMesh.vertexData.u.size() * sizeof(uint16_t);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.u.data(), length);

    // v buffer
    offset += length;
    length = quantizedMesh.vertexData.v.size() * sizeof(uint16_t);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.v.data(), length);

    // height buffer
    offset += length;
    length = quantizedMesh.vertexData.height.size() * sizeof(uint16_t);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.height.data(), length);

    // triangle count
    offset += length;
    uint32_t triangleCount = static_cast<uint32_t>(quantizedMesh.vertexData.indices.size()) / 3;
    length = sizeof(triangleCount);
    std::memcpy(buffer.data() + offset, &triangleCount, length);

    // indices buffer
    offset += length;
    length = quantizedMesh.vertexData.indices.size() * sizeof(T);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.indices.data(), length);

    // west edge count
    offset += length;
    uint32_t westIndicesCount = static_cast<uint32_t>(quantizedMesh.vertexData.westIndices.size());
    length = sizeof(westIndicesCount);
    std::memcpy(buffer.data() + offset, &westIndicesCount, length);

    // west edge buffer
    offset += length;
    length = quantizedMesh.vertexData.westIndices.size() * sizeof(T);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.westIndices.data(), length);

    // south edge count
    offset += length;
    uint32_t southIndicesCount = static_cast<uint32_t>(quantizedMesh.vertexData.southIndices.size());
    length = sizeof(southIndicesCount);
    std::memcpy(buffer.data() + offset, &southIndicesCount, length);

    // south edge buffer
    offset += length;
    length = quantizedMesh.vertexData.southIndices.size() * sizeof(T);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.southIndices.data(), length);

    // east edge count
    offset += length;
    uint32_t eastIndicesCount = static_cast<uint32_t>(quantizedMesh.vertexData.eastIndices.size());
    length = sizeof(eastIndicesCount);
    std::memcpy(buffer.data() + offset, &eastIndicesCount, length);

    // east edge buffer
    offset += length;
    length = quantizedMesh.vertexData.eastIndices.size() * sizeof(T);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.eastIndices.data(), length);

    // north edge count
    offset += length;
    uint32_t northIndicesCount = static_cast<uint32_t>(quantizedMesh.vertexData.northIndices.size());
    length = sizeof(northIndicesCount);
    std::memcpy(buffer.data() + offset, &northIndicesCount, length);

    // north edge buffer
    offset += length;
    length = quantizedMesh.vertexData.northIndices.size() * sizeof(T);
    std::memcpy(buffer.data() + offset, quantizedMesh.vertexData.northIndices.data(), length);

    // serialize extension
    for (const Extension& extension : quantizedMesh.extensions) {
        // serialize extension ID
        offset += length;
        length = sizeof(extension.extensionID);
        std::memcpy(buffer.data() + offset, &extension.extensionID, length);

        // serialize extension length
        offset += length;
        uint32_t extensionLength = static_cast<uint32_t>(extension.extensionData.size());
        length = sizeof(extensionLength);
        std::memcpy(buffer.data() + offset, &extensionLength, length);

        // serialize extension data
        offset += length;
        length = extension.extensionData.size();
        std::memcpy(buffer.data() + offset, extension.extensionData.data(), length);
    }

    return buffer;
}

template<class T>
static QuantizedMesh<T> createGridQuantizedMesh(const BoundingRegion &region, uint32_t width, uint32_t height) {
    if (width * height > std::numeric_limits<T>::max()) {
        throw std::invalid_argument("Number of vertices can be overflowed");
    }

    QuantizedMesh<T> quantizedMesh;
    const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
    glm::dvec3 center = ellipsoid.cartographicToCartesian(region.getRectangle().computeCenter());
    glm::dvec3 corner = ellipsoid.cartographicToCartesian(region.getRectangle().getNortheast());

    quantizedMesh.header.centerX = center.x;
    quantizedMesh.header.centerY = center.y;
    quantizedMesh.header.centerZ = center.z;

    quantizedMesh.header.minimumHeight = static_cast<float>(region.getMinimumHeight());
    quantizedMesh.header.maximumHeight = static_cast<float>(region.getMaximumHeight());

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
            uint16_t u = static_cast<uint16_t>((static_cast<double>(x) / static_cast<double>(width - 1)) * 32767.0);
            uint16_t v = static_cast<uint16_t>(((static_cast<double>(y) / static_cast<double>(height - 1))) * 32767.0);
            int16_t deltaU = static_cast<int16_t>(u - lastU);
            int16_t deltaV = static_cast<int16_t>(v - lastV);
            quantizedMesh.vertexData.u.emplace_back(zigzagEncode(deltaU));
            quantizedMesh.vertexData.v.emplace_back(zigzagEncode(deltaV));
            quantizedMesh.vertexData.height.push_back(0);
            
            lastU = u;
            lastV = v;

            if (x < width - 1 && y < height - 1) {
                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x+1, y, width)));
                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x, y, width)));
                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x, y+1, width)));

                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x+1, y, width)));
                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x, y+1, width)));
                quantizedMesh.vertexData.indices.emplace_back(static_cast<T>(index2DTo1D(x+1, y+1, width)));
            }

            if (y == 0) {
                quantizedMesh.vertexData.southIndices.emplace_back(static_cast<T>(index2DTo1D(x, y, width)));
            }

            if (y == height - 1) {
                quantizedMesh.vertexData.northIndices.emplace_back(static_cast<T>(index2DTo1D(x, y, width)));
            }

            if (x == 0) {
                quantizedMesh.vertexData.westIndices.emplace_back(static_cast<T>(index2DTo1D(x, y, width)));
            }

            if (x == width - 1) {
                quantizedMesh.vertexData.eastIndices.emplace_back(static_cast<T>(index2DTo1D(x, y, width)));
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

template<class T, class I>
void checkGridMesh(const QuantizedMesh<T> &quantizedMesh, 
    const GltfAccessor<I> &indices, 
    const GltfAccessor<glm::vec3> &positions, 
    const QuadtreeTilingScheme &tilingScheme,
    const Ellipsoid &ellipsoid,
    const Rectangle &tileRectangle,
    uint32_t verticesWidth, 
    uint32_t verticesHeight) 
{
    double west = tileRectangle.minimumX;
    double south = tileRectangle.minimumY;
    double east = tileRectangle.maximumX;
    double north = tileRectangle.maximumY;

    // check grid mesh without skirt
    std::vector<uint16_t> uBuffer = quantizedMesh.vertexData.u;
    std::vector<uint16_t> vBuffer = quantizedMesh.vertexData.v;
    size_t u = 0;
    size_t v = 0;

    std::vector<glm::dvec2> uvs;
    uvs.reserve(verticesWidth * verticesHeight);
    uint32_t positionIdx = 0;
    uint32_t idx = 0;
    for (uint32_t y = 0; y < verticesHeight; ++y) {
        for (uint32_t x = 0; x < verticesWidth; ++x) {
            u += zigZagDecode(uBuffer[positionIdx]);
            v += zigZagDecode(vBuffer[positionIdx]);

            // check that u ratio and v ratio is similar to grid ratio
            double uRatio = static_cast<double>(u) / 32767.0;
            double vRatio = static_cast<double>(v) / 32767.0;
            REQUIRE(Math::equalsEpsilon(uRatio, static_cast<double>(x) / static_cast<double>(verticesWidth - 1), Math::EPSILON4));
            REQUIRE(Math::equalsEpsilon(vRatio, static_cast<double>(y) / static_cast<double>(verticesHeight - 1), Math::EPSILON4));

            // check grid positions
            double longitude = Math::lerp(west, east, uRatio);
            double latitude = Math::lerp(south, north, vRatio);
            glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude));

            glm::dvec3 position = static_cast<glm::dvec3>(positions[positionIdx]);
            position += glm::dvec3(quantizedMesh.header.boundingSphereCenterX, quantizedMesh.header.boundingSphereCenterY, quantizedMesh.header.boundingSphereCenterZ);

            REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::EPSILON3));
            REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::EPSILON3));
            REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::EPSILON3));
            ++positionIdx;

            // check indices
            if (x < verticesWidth - 1 && y < verticesHeight - 1) {
                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x + 1, y, verticesWidth)));
                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x, y, verticesWidth)));
                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x, y + 1, verticesWidth)));

                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x + 1, y, verticesWidth)));
                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x, y + 1, verticesWidth)));
                REQUIRE(indices[idx++] == static_cast<I>(index2DTo1D(x + 1, y + 1, verticesWidth)));
            }

            uvs.emplace_back(uRatio, vRatio);
        }
    }

    // make sure there are skirts in there
    size_t westIndicesCount = quantizedMesh.vertexData.westIndices.size();
    size_t southIndicesCount = quantizedMesh.vertexData.southIndices.size();
    size_t eastIndicesCount = quantizedMesh.vertexData.eastIndices.size();
    size_t northIndicesCount = quantizedMesh.vertexData.northIndices.size();

    size_t gridVerticesCount = verticesWidth * verticesHeight;
    size_t gridIndicesCount = (verticesHeight - 1) * (verticesWidth - 1) * 6;
    size_t totalSkirtVertices = westIndicesCount + southIndicesCount + eastIndicesCount + northIndicesCount;
    size_t totalSkirtIndices = (totalSkirtVertices - 4) * 6;

    double skirtHeight = calculateSkirtHeight(10, ellipsoid, tilingScheme);
    double longitudeOffset = (west - east) * 0.0001;
    double latitudeOffset = (north - south) * 0.0001;

    REQUIRE(totalSkirtIndices == indices.size() - gridIndicesCount);
    REQUIRE(totalSkirtVertices == positions.size() - gridVerticesCount);

    size_t currentVertexCount = gridVerticesCount;
    for (size_t i = 0; i < westIndicesCount; ++i) {
        T westIndex = quantizedMesh.vertexData.westIndices[i];
        double longitude = west + longitudeOffset;
        double latitude = Math::lerp(south, north, uvs[westIndex].y);
        glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, -skirtHeight));

        glm::dvec3 position = static_cast<glm::dvec3>(positions[currentVertexCount + i]);
        position += glm::dvec3(quantizedMesh.header.boundingSphereCenterX, quantizedMesh.header.boundingSphereCenterY, quantizedMesh.header.boundingSphereCenterZ);
        REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::EPSILON3));
    }

    currentVertexCount += westIndicesCount;
    for (size_t i = 0; i < southIndicesCount; ++i) {
        T southIndex = quantizedMesh.vertexData.southIndices[southIndicesCount-1 - i];
        double longitude = Math::lerp(west, east, uvs[southIndex].x);
        double latitude = south - latitudeOffset;
        glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, -skirtHeight));

        glm::dvec3 position = static_cast<glm::dvec3>(positions[currentVertexCount + i]);
        position += glm::dvec3(quantizedMesh.header.boundingSphereCenterX, quantizedMesh.header.boundingSphereCenterY, quantizedMesh.header.boundingSphereCenterZ);
        REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::EPSILON3));
    }

    currentVertexCount += southIndicesCount;
    for (size_t i = 0; i < eastIndicesCount; ++i) {
        T eastIndex = quantizedMesh.vertexData.eastIndices[eastIndicesCount-1 - i];
        double longitude = east + longitudeOffset;
        double latitude = Math::lerp(south, north, uvs[eastIndex].y);
        glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, -skirtHeight));

        glm::dvec3 position = static_cast<glm::dvec3>(positions[currentVertexCount + i]);
        position += glm::dvec3(quantizedMesh.header.boundingSphereCenterX, quantizedMesh.header.boundingSphereCenterY, quantizedMesh.header.boundingSphereCenterZ);
        REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::EPSILON2));
        REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::EPSILON3));
    }

    currentVertexCount += eastIndicesCount;
    for (size_t i = 0; i < northIndicesCount; ++i) {
        T northIndex = quantizedMesh.vertexData.northIndices[i];
        double longitude = Math::lerp(west, east, uvs[northIndex].x);
        double latitude = north + latitudeOffset;
        glm::dvec3 expectPosition = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, -skirtHeight));

        glm::dvec3 position = static_cast<glm::dvec3>(positions[currentVertexCount + i]);
        position += glm::dvec3(quantizedMesh.header.boundingSphereCenterX, quantizedMesh.header.boundingSphereCenterY, quantizedMesh.header.boundingSphereCenterZ);
        REQUIRE(Math::equalsEpsilon(position.x, expectPosition.x, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.y, expectPosition.y, Math::EPSILON3));
        REQUIRE(Math::equalsEpsilon(position.z, expectPosition.z, Math::EPSILON3));
    }
}

TEST_CASE("Test converting quantized mesh to gltf with skirt") {
    registerAllTileContentTypes();

    // mock context
    Ellipsoid ellipsoid = Ellipsoid::WGS84;
    Rectangle rectangle(glm::radians(-180.0), glm::radians(-90.0), glm::radians(180.0), glm::radians(90.0));
    QuadtreeTilingScheme tilingScheme(rectangle, 2, 1);
    TileContext context{};
    context.implicitContext = ImplicitTilingContext{ 
        std::vector<std::string>{}, 
        tilingScheme, 
        GeographicProjection(ellipsoid),
        QuadtreeTileAvailability(tilingScheme, 23)
    };

    SECTION("Check quantized mesh that has uint16_t indices") {
        // mock quantized mesh
        uint32_t verticesWidth = 3;
        uint32_t verticesHeight = 3;
        QuadtreeTileID tileID(10, 0, 0);
        Rectangle tileRectangle = tilingScheme.tileToRectangle(tileID);
        BoundingRegion boundingVolume = BoundingRegion(
            GlobeRectangle(tileRectangle.minimumX, tileRectangle.minimumY, tileRectangle.maximumX, tileRectangle.maximumY), 
            0.0, 
            0.0);
        QuantizedMesh<uint16_t> quantizedMesh = createGridQuantizedMesh<uint16_t>(boundingVolume, verticesWidth, verticesHeight);
        std::vector<uint8_t> quantizedMeshBin = convertQuantizedMeshToBinary(quantizedMesh);
        gsl::span<const uint8_t> data(quantizedMeshBin.data(), quantizedMeshBin.size());
        std::unique_ptr<TileContentLoadResult> loadResult = TileContentFactory::createContent(context, 
            tileID, 
            boundingVolume, 
            0.0, 
            glm::dmat4(1.0), 
            std::nullopt, 
            TileRefine::Replace, 
            "url", 
            "application/vnd.quantized-mesh", 
            data);
        REQUIRE(loadResult != nullptr);
        REQUIRE(loadResult->model != std::nullopt);

        // make sure the gltf is the grid
        const tinygltf::Model& model = *loadResult->model;
        const tinygltf::Mesh& mesh = model.meshes.front();
        const tinygltf::Primitive& primitive = mesh.primitives.front();

        // make sure mesh contains grid mesh and skirts at the end
        GltfAccessor<uint16_t> indices(model, primitive.indices);
        GltfAccessor<glm::vec3> positions(model, primitive.attributes.at("POSITION"));
        checkGridMesh(quantizedMesh, indices, positions, tilingScheme, ellipsoid, tileRectangle, verticesWidth, verticesHeight);
    }

    SECTION("Check quantized mesh that has uint32_t indices") {
        // mock quantized mesh
        uint32_t verticesWidth = 300;
        uint32_t verticesHeight = 300;
        QuadtreeTileID tileID(10, 0, 0);
        Rectangle tileRectangle = tilingScheme.tileToRectangle(tileID);
        BoundingRegion boundingVolume = BoundingRegion(
            GlobeRectangle(tileRectangle.minimumX, tileRectangle.minimumY, tileRectangle.maximumX, tileRectangle.maximumY), 
            0.0, 
            0.0);
        QuantizedMesh<uint32_t> quantizedMesh = createGridQuantizedMesh<uint32_t>(boundingVolume, verticesWidth, verticesHeight);
        std::vector<uint8_t> quantizedMeshBin = convertQuantizedMeshToBinary(quantizedMesh);
        gsl::span<const uint8_t> data(quantizedMeshBin.data(), quantizedMeshBin.size());
        std::unique_ptr<TileContentLoadResult> loadResult = TileContentFactory::createContent(context, 
            tileID, 
            boundingVolume, 
            0.0, 
            glm::dmat4(1.0), 
            std::nullopt, 
            TileRefine::Replace, 
            "url", 
            "application/vnd.quantized-mesh", 
            data);
        REQUIRE(loadResult != nullptr);
        REQUIRE(loadResult->model != std::nullopt);

        // make sure the gltf is the grid
        const tinygltf::Model& model = *loadResult->model;
        const tinygltf::Mesh& mesh = model.meshes.front();
        const tinygltf::Primitive& primitive = mesh.primitives.front();

        // make sure mesh contains grid mesh and skirts at the end
        GltfAccessor<uint32_t> indices(model, primitive.indices);
        GltfAccessor<glm::vec3> positions(model, primitive.attributes.at("POSITION"));
        checkGridMesh(quantizedMesh, indices, positions, tilingScheme, ellipsoid, tileRectangle, verticesWidth, verticesHeight);
    }
}

