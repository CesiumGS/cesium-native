#include "QuantizedMesh.h"
#include "CesiumUtility/Math.h"
#include "CesiumGeospatial/Rectangle.h"
#include "Cesium3DTiles/Tile.h"
#include "tiny_gltf.h"
#include <stdexcept>
#include <glm/vec3.hpp>

using namespace CesiumUtility;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {
    std::string QuantizedMesh::CONTENT_TYPE = "application/vnd.quantized-mesh";

    struct QuantizedMeshHeader
    {
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

        // The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
        // If this point is below the horizon, the entire tile is below the horizon.
        // See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
        double HorizonOcclusionPointX;
        double HorizonOcclusionPointY;
        double HorizonOcclusionPointZ;

        // The total number of vertices.
        uint32_t vertexCount;
    };

    // We can't use sizeof(QuantizedMeshHeader) because it may be padded.
    const size_t headerLength = 92;

    int32_t zigZagDecode(int32_t value) {
        return (value >> 1) ^ (-(value & 1));
    }

    template <class T>
    void decodeIndices(const gsl::span<const T>& encoded, gsl::span<T>& decoded) {
        if (decoded.size() < encoded.size()) {
            throw std::runtime_error("decoded buffer is too small.");
        }

        T highest = 0;
        for (size_t i = 0; i < encoded.size(); ++i) {
            T code = encoded[i];
            decoded[i] = highest - code;
            if (code == 0) {
                ++highest;
            }
        }
    }

    std::unique_ptr<GltfContent> QuantizedMesh::load(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
        if (data.size() < headerLength) {
            return nullptr;
        }

        size_t readIndex = 0;

        const QuantizedMeshHeader* pHeader = reinterpret_cast<const QuantizedMeshHeader*>(data.data());
        glm::dvec3 center(pHeader->BoundingSphereCenterX, pHeader->BoundingSphereCenterY, pHeader->BoundingSphereCenterZ);
        glm::dvec3 horizonOcclusionPoint(pHeader->HorizonOcclusionPointX, pHeader->HorizonOcclusionPointY, pHeader->HorizonOcclusionPointZ);
        double minimumHeight = pHeader->MinimumHeight;
        double maximumHeight = pHeader->MaximumHeight;

        uint32_t vertexCount = pHeader->vertexCount;

        readIndex += headerLength;

        const gsl::span<const uint16_t> uBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += uBuffer.size_bytes();
        if (readIndex > data.size()) {
            return nullptr;
        }

        const gsl::span<const uint16_t> vBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += vBuffer.size_bytes();
        if (readIndex > data.size()) {
            return nullptr;
        }

        const gsl::span<const uint16_t> heightBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += heightBuffer.size_bytes();
        if (readIndex > data.size()) {
            return nullptr;
        }

        int32_t u = 0;
        int32_t v = 0;
        int32_t height = 0;

        const CesiumGeospatial::Rectangle& rectangle = std::get<BoundingRegion>(tile.getBoundingVolume()).getRectangle();
        double west = rectangle.getWest();
        double south = rectangle.getSouth();
        double east = rectangle.getEast();
        double north = rectangle.getNorth();

        tinygltf::Model model;
        
        int positionBufferId = static_cast<int>(model.buffers.size());
        model.buffers.emplace_back();
        tinygltf::Buffer& positionBuffer = model.buffers[positionBufferId];
        positionBuffer.data.resize(vertexCount * 3 * sizeof(float));

        int positionBufferViewId = static_cast<int>(model.bufferViews.size());
        model.bufferViews.emplace_back();
        tinygltf::BufferView& positionBufferView = model.bufferViews[positionBufferViewId];
        positionBufferView.buffer = positionBufferId;
        positionBufferView.byteOffset = 0;
        positionBufferView.byteStride = 3 * sizeof(float);
        positionBufferView.byteLength = positionBuffer.data.size();
        positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        int positionAccessorId = static_cast<int>(model.accessors.size());
        model.accessors.emplace_back();
        tinygltf::Accessor& positionAccessor = model.accessors[positionAccessorId];
        positionAccessor.bufferView = positionBufferViewId;
        positionAccessor.byteOffset = 0;
        positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        positionAccessor.count = vertexCount;
        positionAccessor.type = TINYGLTF_TYPE_VEC3;
        
        int meshId = static_cast<int>(model.meshes.size());
        model.meshes.emplace_back();
        tinygltf::Mesh& mesh = model.meshes[meshId];
        mesh.primitives.emplace_back();

        tinygltf::Primitive& primitive = mesh.primitives[0];
        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        primitive.attributes.emplace("POSITION", positionAccessorId);
        primitive.material = 0;

        // model.buffers.push_back(tinygltf::Buffer());
        // tinygltf::Buffer& indexBuffer = model.buffers[model.buffers.size() - 1];

        float* pPositions = reinterpret_cast<float*>(positionBuffer.data.data());
        size_t positionOutputIndex = 0;

        const Ellipsoid& ellipsoid = Ellipsoid::WGS84;

        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double minZ = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();
        double maxZ = std::numeric_limits<double>::lowest();

        for (size_t i = 0; i < vertexCount; ++i) {
            u += zigZagDecode(uBuffer[i]);
            v += zigZagDecode(vBuffer[i]);
            height += zigZagDecode(heightBuffer[i]);

            double longitude = Math::lerp(west, east, static_cast<double>(u) / 32767.0);
            double latitude = Math::lerp(south, north, static_cast<double>(v) / 32767.0);
            double heightMeters = Math::lerp(minimumHeight, maximumHeight, static_cast<double>(height) / 32767.0);

            glm::dvec3 position = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, heightMeters));
            position -= center;
            pPositions[positionOutputIndex++] = static_cast<float>(position.x);
            pPositions[positionOutputIndex++] = static_cast<float>(position.y);
            pPositions[positionOutputIndex++] = static_cast<float>(position.z);

            minX = std::min(minX, position.x);
            minY = std::min(minY, position.y);
            minZ = std::min(minZ, position.z);

            maxX = std::max(maxX, position.x);
            maxY = std::max(maxY, position.y);
            maxZ = std::max(maxZ, position.z);
        }

        positionAccessor.minValues = { minX, minY, minZ };
        positionAccessor.maxValues = { maxX, maxY, maxZ };

        int indicesBufferId = static_cast<int>(model.buffers.size());
        model.buffers.emplace_back();
        tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferId];
        
        int indicesBufferViewId = static_cast<int>(model.bufferViews.size());
        model.bufferViews.emplace_back();
        tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesBufferViewId];
        indicesBufferView.buffer = indicesBufferId;
        indicesBufferView.byteOffset = 0;
        positionBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

        int indicesAccessorId = static_cast<int>(model.accessors.size());
        model.accessors.emplace_back();
        tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorId];
        indicesAccessor.bufferView = indicesBufferViewId;
        indicesAccessor.byteOffset = 0;
        indicesAccessor.type = TINYGLTF_TYPE_SCALAR;

        primitive.indices = indicesBufferId;

        if (pHeader->vertexCount > 65536) {
            // 32-bit indices
            if ((readIndex % 4) != 0) {
                readIndex += 2;
                if (readIndex > data.size()) {
                    return nullptr;
                }
            }

            uint32_t triangleCount = *reinterpret_cast<const uint32_t*>(data.data() + readIndex);
            readIndex += sizeof(uint32_t);
            if (readIndex > data.size()) {
                return nullptr;
            }

            const gsl::span<const uint32_t> indices(reinterpret_cast<const uint32_t*>(data.data() + readIndex), triangleCount * 3);
            readIndex += indices.size_bytes();
            if (readIndex > data.size()) {
                return nullptr;
            }

            indicesBuffer.data.resize(triangleCount * 3 * sizeof(uint32_t));
            indicesBufferView.byteLength = indicesBuffer.data.size();
            indicesBufferView.byteStride = sizeof(uint32_t);
            indicesAccessor.count = triangleCount * 3;
            indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;

            gsl::span<uint32_t> outputIndices(reinterpret_cast<uint32_t*>(indicesBuffer.data.data()), indicesBuffer.data.size() / sizeof(uint32_t));
            decodeIndices(indices, outputIndices);
        } else {
            // 16-bit indices
            uint32_t triangleCount = *reinterpret_cast<const uint32_t*>(data.data() + readIndex);
            readIndex += sizeof(uint32_t);
            if (readIndex > data.size()) {
                return nullptr;
            }

            const gsl::span<const uint16_t> indices(reinterpret_cast<const uint16_t*>(data.data() + readIndex), triangleCount * 3);
            readIndex += indices.size_bytes();
            if (readIndex > data.size()) {
                return nullptr;
            }

            indicesBuffer.data.resize(triangleCount * 3 * sizeof(uint16_t));
            indicesBufferView.byteLength = indicesBuffer.data.size();
            indicesBufferView.byteStride = sizeof(uint16_t);
            indicesAccessor.count = triangleCount * 3;
            indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;

            gsl::span<uint16_t> outputIndices(reinterpret_cast<uint16_t*>(indicesBuffer.data.data()), indicesBuffer.data.size() / sizeof(uint16_t));
            decodeIndices(indices, outputIndices);
        }

        return std::make_unique<GltfContent>(tile, std::move(model), url);
    }

}
