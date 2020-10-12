#include "Cesium3DTiles/GltfAccessor.h"
#include "CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h"
#include "CesiumUtility/Math.h"
#include "upsampleGltfForRasterOverlays.h"

namespace Cesium3DTiles {

    static void upsamplePrimitiveForRasterOverlays(
        const tinygltf::Model& parentModel,
        tinygltf::Model& model,
        tinygltf::Mesh& mesh,
        tinygltf::Primitive& primitive,
        CesiumGeometry::QuadtreeChild childID
    );

    tinygltf::Model upsampleGltfForRasterOverlays(const tinygltf::Model& parentModel, CesiumGeometry::QuadtreeChild childID) {
        tinygltf::Model result;

        // Copy the entire parent model except for the meshes/primitives, buffers,
        // bufferViews, and accessors, which we'll be rewriting.
        result.accessors = parentModel.accessors;
        result.animations = parentModel.animations;
        result.materials = parentModel.materials;
        result.nodes = parentModel.nodes;
        result.textures = parentModel.textures;
        result.images = parentModel.images;
        result.skins = parentModel.skins;
        result.samplers = parentModel.samplers;
        result.cameras = parentModel.cameras;
        result.scenes = parentModel.scenes;
        result.lights = parentModel.lights;
        result.defaultScene = parentModel.defaultScene;
        result.extensionsUsed = parentModel.extensionsUsed;
        result.extensionsRequired = parentModel.extensionsRequired;
        result.asset = parentModel.asset;
        result.extras = parentModel.extras;
        result.extensions = parentModel.extensions;
        result.extras_json_string = parentModel.extras_json_string;
        result.extensions_json_string = parentModel.extensions_json_string;

        for (tinygltf::Mesh& mesh : result.meshes) {
            for (tinygltf::Primitive& primitive : mesh.primitives) {
                upsamplePrimitiveForRasterOverlays(parentModel, result, mesh, primitive, childID);
            }
        }

        return result;
    }

    struct FloatVertexAttribute {
        const std::vector<unsigned char>& buffer;
        size_t offset;
        int32_t stride;
        int32_t numberOfFloatsPerVertex;
    };

    static void copyVertexAttributes(
        const std::vector<FloatVertexAttribute>& vertexAttributes,
        const CesiumGeometry::TriangleClipVertex& vertex,
        gsl::span<float> output,
        uint32_t& outputIndex
    ) {
        struct Operation {
            const std::vector<FloatVertexAttribute>& vertexAttributes;
            gsl::span<float>& output;
            uint32_t& outputIndex;

            void operator()(int vertexIndex) {
                for (const FloatVertexAttribute& attribute : vertexAttributes) {
                    const float* pInput = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertexIndex);
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        output[outputIndex] = *pInput;
                        ++outputIndex;
                    }
                    
                }
            }

            void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                for (const FloatVertexAttribute& attribute : vertexAttributes) {
                    const float* pInput0 = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertex.first);
                    const float* pInput1 = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertex.second);
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        output[outputIndex] = glm::mix(*pInput0, *pInput1, vertex.t);
                        ++outputIndex;
                    }
                    
                }
            }
        };

        std::visit(Operation { vertexAttributes, output, outputIndex }, vertex);
    }

    static void copyVertexAttributes(
        const std::vector<FloatVertexAttribute>& vertexAttributes,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const CesiumGeometry::TriangleClipVertex& vertex,
        gsl::span<float> output,
        uint32_t& outputIndex
    ) {
        struct Operation {
            const std::vector<FloatVertexAttribute>& vertexAttributes;
            const std::vector<CesiumGeometry::TriangleClipVertex>& complements;
            gsl::span<float>& output;
            uint32_t& outputIndex;

            void operator()(int vertexIndex) {
                if (vertexIndex < 0) {
                    copyVertexAttributes(vertexAttributes, complements[~vertexIndex], output, outputIndex);
                } else {
                    copyVertexAttributes(vertexAttributes, vertexIndex, output, outputIndex);
                }
            }

            void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                const int maxFloats = 100;
                float temp0[maxFloats];
                float temp1[maxFloats];
                uint32_t outputIndex0 = 0;
                uint32_t outputIndex1 = 0;

                if (vertex.first < 0) {
                    copyVertexAttributes(vertexAttributes, complements[~vertex.first], gsl::span(temp0, maxFloats), outputIndex0);
                } else {
                    copyVertexAttributes(vertexAttributes, vertex.first, gsl::span(temp0, maxFloats), outputIndex0);
                }

                
                if (vertex.second < 0) {
                    copyVertexAttributes(vertexAttributes, complements[~vertex.second], gsl::span(temp1, maxFloats), outputIndex1);
                } else {
                    copyVertexAttributes(vertexAttributes, vertex.second, gsl::span(temp1, maxFloats), outputIndex1);
                }

                const float* pInput0 = temp0;
                const float* pInput1 = temp1;

                for (const FloatVertexAttribute& attribute : vertexAttributes) {
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        output[outputIndex] = glm::mix(*pInput0, *pInput1, vertex.t);
                        ++outputIndex;
                    }
                }
            }
        };

        std::visit(Operation { vertexAttributes, complements, output, outputIndex }, vertex);
    }

    template <class T>
    static T getVertexValue(const GltfAccessor<T>& accessor, const CesiumGeometry::TriangleClipVertex& vertex) {
        struct Operation {
            const Cesium3DTiles::GltfAccessor<T>& accessor;

            T operator()(int vertexIndex) {
                return accessor[vertexIndex];
            }

            T operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                const T& v0 = accessor[vertex.first];
                const T& v1 = accessor[vertex.second];
                return glm::mix(v0, v1, vertex.t);
            }
        };

        return std::visit(Operation { accessor }, vertex);
    }

    template <class T>
    static T getVertexValue(const Cesium3DTiles::GltfAccessor<T>& accessor, const std::vector<CesiumGeometry::TriangleClipVertex>& complements, const CesiumGeometry::TriangleClipVertex& vertex) {
        struct Operation {
            const Cesium3DTiles::GltfAccessor<T>& accessor;
            const std::vector<CesiumGeometry::TriangleClipVertex>& complements;

            TA_CENTER operator()(int vertexIndex) {
                return get(vertexIndex);
            }

            T operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                T v0 = get(vertex.first);
                T v1 = get(vertex.second);
                return glm::mix(v0, v1, vertex.t);
            }

            T get(int vertexIndex) {
                if (vertexIndex < 0) {
                    return getVertexValue(accessor, complements[~vertexIndex]);
                }
                return accessor[vertexIndex];
            }
        };

        return std::visit(Operation { accessor, complements }, vertex);
    }

    template <class TIndex>
    static void upsamplePrimitiveForRasterOverlays(
        const tinygltf::Model& parentModel,
        tinygltf::Model& model,
        tinygltf::Mesh& /*mesh*/,
        tinygltf::Primitive& primitive,
        CesiumGeometry::QuadtreeChild childID
    ) {
        // Add up the per-vertex size of all attributes and create buffers, bufferViews, and accessors
        std::vector<FloatVertexAttribute> attributes;
        attributes.reserve(primitive.attributes.size());

        size_t vertexBufferIndex = model.buffers.size();
        model.buffers.emplace_back();

        size_t indexBufferIndex = model.buffers.size();
        model.buffers.emplace_back();

        // tinygltf::Buffer& vertexBuffer = model.buffers[vertexBufferIndex];
        // tinygltf::Buffer& indexBuffer = model.buffers[indexBufferIndex];

        size_t vertexBufferViewIndex = model.bufferViews.size();
        model.bufferViews.emplace_back();

        size_t indexBufferViewIndex = model.bufferViews.size();
        model.bufferViews.emplace_back();

        tinygltf::BufferView& vertexBufferView = model.bufferViews[vertexBufferViewIndex];
        vertexBufferView.buffer = static_cast<int>(vertexBufferIndex);

        tinygltf::BufferView& indexBufferView = model.bufferViews[indexBufferViewIndex];
        indexBufferView.buffer = static_cast<int>(indexBufferIndex);

        uint32_t vertexSizeFloats = 0;
        int uvAccessorIndex = -1;

        for (std::pair<const std::string, int>& attribute : primitive.attributes) {
            if (attribute.first == "_CESIUMOVERLAY_0") {
                uvAccessorIndex = attribute.second;
            }

            if (attribute.second < 0 || attribute.second >= parentModel.accessors.size()) {
                continue;
            }

            const tinygltf::Accessor& accessor = parentModel.accessors[attribute.second];
            if (accessor.bufferView < 0 || accessor.bufferView >= parentModel.bufferViews.size()) {
                continue;
            }

            const tinygltf::BufferView& bufferView = parentModel.bufferViews[accessor.bufferView];
            if (bufferView.buffer < 0 || bufferView.buffer >= parentModel.buffers.size()) {
                continue;
            }

            const tinygltf::Buffer& buffer = parentModel.buffers[bufferView.buffer];

            int32_t accessorByteStride = accessor.ByteStride(bufferView);
			int32_t accessorComponentElements = tinygltf::GetNumComponentsInType(accessor.type);
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                // Can only interpolate floating point vertex attributes
                return;
            }

            attribute.second = static_cast<int>(model.accessors.size());
            model.accessors.emplace_back();
            tinygltf::Accessor& newAccessor = model.accessors.back();
            newAccessor.bufferView = static_cast<int>(vertexBufferIndex);
            newAccessor.byteOffset = vertexSizeFloats * sizeof(float);
            // TODO: newAccessor.count
            newAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            newAccessor.type = accessor.type;

            vertexSizeFloats += accessorComponentElements;

            attributes.push_back(FloatVertexAttribute {
                buffer.data,
                accessor.byteOffset,
                accessorByteStride,
                accessorComponentElements
            });

        }

        if (uvAccessorIndex == -1) {
            // We don't know how to divide this primitive, so just copy it verbatim.
            // TODO
            return;
        }

        bool keepAboveU = childID == CesiumGeometry::QuadtreeChild::LowerRight || childID == CesiumGeometry::QuadtreeChild::UpperRight;
        bool keepAboveV = childID == CesiumGeometry::QuadtreeChild::UpperLeft || childID == CesiumGeometry::QuadtreeChild::UpperRight;

        GltfAccessor<glm::vec2> uvAccessor(parentModel, uvAccessorIndex);
        GltfAccessor<TIndex> indicesAccessor(parentModel, primitive.indices);

        std::vector<CesiumGeometry::TriangleClipVertex> clippedA;
        std::vector<CesiumGeometry::TriangleClipVertex> clippedB;

        // Maps old (parentModel) vertex indices to new (model) vertex indices.
        std::vector<uint32_t> vertexMap(uvAccessor.size(), std::numeric_limits<uint32_t>::max());

        std::vector<unsigned char> newVertexBuffer(vertexSizeFloats * sizeof(float));
        gsl::span<float> newVertexFloats(reinterpret_cast<float*>(newVertexBuffer.data()), newVertexBuffer.size() / sizeof(float));
        uint32_t nextWriteIndex = 0;
        std::vector<uint32_t> indices;

        for (size_t i = 0; i < indicesAccessor.size(); i += 3) {
            TIndex i0 = indicesAccessor[i];
            TIndex i1 = indicesAccessor[i + 1];
            TIndex i2 = indicesAccessor[i + 2];

            glm::vec2 uv0 = uvAccessor[i0];
            glm::vec2 uv1 = uvAccessor[i1];
            glm::vec2 uv2 = uvAccessor[i2];

            // Clip this triangle against the East-West boundary
            clippedA.clear();
            clipTriangleAtAxisAlignedThreshold(0.5, keepAboveU, i0, i1, i2, uv0.x, uv1.x, uv2.x, clippedA);

            if (clippedA.size() < 3) {
                // No part of this triangle is inside the target tile.
                continue;
            }

            // Clip the first clipped triange against the North-South boundary
            clippedB.clear();
            clipTriangleAtAxisAlignedThreshold(
                0.5,
                keepAboveV,
                ~0,
                ~1,
                ~2,
                getVertexValue(uvAccessor, clippedA[0]).y,
                getVertexValue(uvAccessor, clippedA[1]).y,
                getVertexValue(uvAccessor, clippedA[2]).y,
                clippedB
            );

            // Add the clipped triangle or quad, if any
            addClippedPolygon(newVertexFloats, nextWriteIndex, indices, attributes, vertexMap, clippedA, clippedB);

            // If the East-West clip yielded a quad (rather than a triangle), clip the second triangle of the quad, too.
            if (clippedA.size() > 3) {
                clippedB.clear();
                clipTriangleAtAxisAlignedThreshold(
                    0.5,
                    keepAboveV,
                    ~0,
                    ~1,
                    ~2,
                    getVertexValue(uvAccessor, clippedA[0]).y,
                    getVertexValue(uvAccessor, clippedA[2]).y,
                    getVertexValue(uvAccessor, clippedA[3]).y,
                    clippedB
                );

                // Add the clipped triangle or quad, if any
                addClippedPolygon(newVertexFloats, nextWriteIndex, indices, attributes, vertexMap, clippedA, clippedB);
            }
        }
    }

    static uint32_t getOrCreateVertex(
        gsl::span<float>& output,
        uint32_t& outputIndex,
        const std::vector<FloatVertexAttribute>& attributes,
        std::vector<uint32_t>& vertexMap,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const CesiumGeometry::TriangleClipVertex& clipVertex
    ) {
        const int* pIndex = std::get_if<int>(&clipVertex);
        if (pIndex) {
            uint32_t existingIndex = vertexMap[*pIndex];
            if (existingIndex != std::numeric_limits<uint32_t>::max()) {
                return existingIndex;
            }
        }

        uint32_t beforeOutput = outputIndex;
        copyVertexAttributes(attributes, complements, clipVertex, output, outputIndex);
        uint32_t newIndex = beforeOutput / (outputIndex - beforeOutput);

        if (pIndex) {
            vertexMap[*pIndex] = newIndex;
        }

        return newIndex;
    }

    static void addClippedPolygon(
        gsl::span<float>& output,
        uint32_t& outputIndex,
        std::vector<uint32_t>& indices,
        const std::vector<FloatVertexAttribute>& attributes,
        std::vector<uint32_t>& vertexMap,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult
    ) {
        if (clipResult.size() < 3) {
            return;
        }

        uint32_t i0 = getOrCreateVertex(output, outputIndex, attributes, vertexMap, complements, clipResult[0]);
        uint32_t i1 = getOrCreateVertex(output, outputIndex, attributes, vertexMap, complements, clipResult[1]);
        uint32_t i2 = getOrCreateVertex(output, outputIndex, attributes, vertexMap, complements, clipResult[2]);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        if (clipResult.size() > 3) {

        }
    }

    static void upsamplePrimitiveForRasterOverlays(
        const tinygltf::Model& parentModel,
        tinygltf::Model& model,
        tinygltf::Mesh& mesh,
        tinygltf::Primitive& primitive,
        CesiumGeometry::QuadtreeChild childID
    ) {
        if (
            primitive.mode != TINYGLTF_MODE_TRIANGLES ||
            primitive.indices < 0 ||
            primitive.indices >= model.accessors.size()
        ) {
            // Not indexed triangles, so we don't know how to divide this primitive (yet).
            // So just copy it verbatim.
            // TODO
            return;
        }

        tinygltf::Accessor& indicesAccessorGltf = model.accessors[primitive.indices];
        if (indicesAccessorGltf.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            upsamplePrimitiveForRasterOverlays<uint16_t>(parentModel, model, mesh, primitive, childID);
        } else if (indicesAccessorGltf.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            upsamplePrimitiveForRasterOverlays<uint32_t>(parentModel, model, mesh, primitive, childID);
        }
    }

}
