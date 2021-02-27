// Copyright CesiumGS, Inc. and Contributors

#include "CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumUtility/Math.h"
#include "SkirtMeshMetadata.h"
#include "upsampleGltfForRasterOverlays.h"
#include <algorithm>

using namespace CesiumGltf;

namespace Cesium3DTiles {
    struct EdgeVertex {
        uint32_t index;
        glm::vec2 uv;
    };

    struct EdgeIndices {
        std::vector<EdgeVertex> west;
        std::vector<EdgeVertex> south;
        std::vector<EdgeVertex> east;
        std::vector<EdgeVertex> north;
    };

    static void upsamplePrimitiveForRasterOverlays(
        const Model& parentModel,
        Model& model,
        Mesh& mesh,
        MeshPrimitive& primitive,
        CesiumGeometry::UpsampledQuadtreeNode childID
    );

    struct FloatVertexAttribute {
        const std::vector<unsigned char>& buffer;
        int64_t offset;
        int64_t stride;
        int64_t numberOfFloatsPerVertex;
        int32_t accessorIndex;
        std::vector<double> minimums;
        std::vector<double> maximums;
    };

    static void addClippedPolygon(
        std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        std::vector<uint32_t>& vertexMap,
        std::vector<uint32_t>& clipVertexToIndices,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult
    );

    static void addEdge(EdgeIndices& edgeIndices,
        double thresholdU,
        double thresholdV,
        bool keepAboveU,
        bool keepAboveV,
        const AccessorView<glm::vec2>& uvs,
        const std::vector<uint32_t>& clipVertexToIndices,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult);

    static void addSkirt(std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        const std::vector<uint32_t>& edgeIndices,
        const glm::dvec3& center,
        double skirtHeight,
        int64_t vertexSizeFloats,
        int32_t positionAttributeIndex);

    static void addSkirts(std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        CesiumGeometry::UpsampledQuadtreeNode childID,
        SkirtMeshMetadata &currentSkirt,
        const SkirtMeshMetadata &parentSkirt,
        EdgeIndices &edgeIndices,
        int64_t vertexSizeFloats,
        int32_t positionAttributeIndex);

    static bool isWestChild(CesiumGeometry::UpsampledQuadtreeNode childID) {
        return (childID.tileID.x % 2) == 0;
    }

    static bool isSouthChild(CesiumGeometry::UpsampledQuadtreeNode childID) {
        return (childID.tileID.y % 2) == 0;
    }

    Model upsampleGltfForRasterOverlays(const Model& parentModel, CesiumGeometry::UpsampledQuadtreeNode childID) {
        Model result;

        // Copy the entire parent model except for the buffers, bufferViews, and accessors, which we'll be rewriting.
        result.animations = parentModel.animations;
        result.materials = parentModel.materials;
        result.meshes = parentModel.meshes;
        result.nodes = parentModel.nodes;
        result.textures = parentModel.textures;
        result.images = parentModel.images;
        result.skins = parentModel.skins;
        result.samplers = parentModel.samplers;
        result.cameras = parentModel.cameras;
        result.scenes = parentModel.scenes;
        result.scene = parentModel.scene;
        result.extensionsUsed = parentModel.extensionsUsed;
        result.extensionsRequired = parentModel.extensionsRequired;
        result.asset = parentModel.asset;
        // result.extras = parentModel.extras;
        // result.extensions = parentModel.extensions;
        // result.extras_json_string = parentModel.extras_json_string;
        // result.extensions_json_string = parentModel.extensions_json_string;

        for (Mesh& mesh : result.meshes) {
            for (MeshPrimitive& primitive : mesh.primitives) {
                upsamplePrimitiveForRasterOverlays(parentModel, result, mesh, primitive, childID);
            }
        }

        return result;
    }

    static void copyVertexAttributes(
        std::vector<FloatVertexAttribute>& vertexAttributes,
        const CesiumGeometry::TriangleClipVertex& vertex,
        std::vector<float>& output,
        bool skipMinMaxUpdate = false
    ) {
        struct Operation {
            std::vector<FloatVertexAttribute>& vertexAttributes;
            std::vector<float>& output;
            bool skipMinMaxUpdate;

            void operator()(int vertexIndex) {
                for (FloatVertexAttribute& attribute : vertexAttributes) {
                    const float* pInput = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertexIndex);
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        float value = *pInput;
                        output.push_back(value);
                        if (!skipMinMaxUpdate) {
                            attribute.minimums[static_cast<size_t>(i)] = glm::min(attribute.minimums[static_cast<size_t>(i)], static_cast<double>(value));
                            attribute.maximums[static_cast<size_t>(i)] = glm::max(attribute.maximums[static_cast<size_t>(i)], static_cast<double>(value));
                        }
                        ++pInput;
                    }
                    
                }
            }

            void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                for (FloatVertexAttribute& attribute : vertexAttributes) {
                    const float* pInput0 = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertex.first);
                    const float* pInput1 = reinterpret_cast<const float*>(attribute.buffer.data() + attribute.offset + attribute.stride * vertex.second);
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        float value = glm::mix(*pInput0, *pInput1, vertex.t);
                        output.push_back(value);
                        if (!skipMinMaxUpdate) {
                            attribute.minimums[static_cast<size_t>(i)] = glm::min(attribute.minimums[static_cast<size_t>(i)], static_cast<double>(value));
                            attribute.maximums[static_cast<size_t>(i)] = glm::max(attribute.maximums[static_cast<size_t>(i)], static_cast<double>(value));
                        }
                        ++pInput0;
                        ++pInput1;
                    }
                    
                }
            }
        };

        std::visit(Operation { vertexAttributes, output, skipMinMaxUpdate }, vertex);
    }

    static void copyVertexAttributes(
        std::vector<FloatVertexAttribute>& vertexAttributes,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const CesiumGeometry::TriangleClipVertex& vertex,
        std::vector<float>& output
    ) {
        struct Operation {
            std::vector<FloatVertexAttribute>& vertexAttributes;
            const std::vector<CesiumGeometry::TriangleClipVertex>& complements;
            std::vector<float>& output;

            void operator()(int vertexIndex) {
                if (vertexIndex < 0) {
                    copyVertexAttributes(vertexAttributes, complements[static_cast<size_t>(~vertexIndex)], output);
                } else {
                    copyVertexAttributes(vertexAttributes, vertexIndex, output);
                }
            }

            void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                size_t outputIndex0 = output.size();

                // Copy the two vertices into the output array
                if (vertex.first < 0) {
                    copyVertexAttributes(vertexAttributes, complements[static_cast<size_t>(~vertex.first)], output, true);
                } else {
                    copyVertexAttributes(vertexAttributes, vertex.first, output, true);
                }

                size_t outputIndex1 = output.size();
                
                if (vertex.second < 0) {
                    copyVertexAttributes(vertexAttributes, complements[static_cast<size_t>(~vertex.second)], output, true);
                } else {
                    copyVertexAttributes(vertexAttributes, vertex.second, output, true);
                }

                // Interpolate between them and overwrite the first with the result.
                for (FloatVertexAttribute& attribute : vertexAttributes) {
                    for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
                        float value = glm::mix(output[outputIndex0], output[outputIndex1], vertex.t);
                        output[outputIndex0] = value;
                        attribute.minimums[static_cast<size_t>(i)] = glm::min(attribute.minimums[static_cast<size_t>(i)], static_cast<double>(value));
                        attribute.maximums[static_cast<size_t>(i)] = glm::max(attribute.maximums[static_cast<size_t>(i)], static_cast<double>(value));
                        ++outputIndex0;
                        ++outputIndex1;
                    }
                }

                // Remove the temporary second, which is now pointed to be outputIndex0.
                output.erase(output.begin() + static_cast<std::vector<float>::iterator::difference_type>(outputIndex0), output.end());
            }
        };

        std::visit(Operation { vertexAttributes, complements, output, }, vertex);
    }

    template <class T>
    static T getVertexValue(const AccessorView<T>& accessor, const CesiumGeometry::TriangleClipVertex& vertex) {
        struct Operation {
            const AccessorView<T>& accessor;

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
    static T getVertexValue(const AccessorView<T>& accessor, 
        const std::vector<CesiumGeometry::TriangleClipVertex> &complements, 
        const CesiumGeometry::TriangleClipVertex& vertex) 
    {
        struct Operation {
            const AccessorView<T>& accessor;
            const std::vector<CesiumGeometry::TriangleClipVertex>& complements;

            T operator()(int vertexIndex) {
                if (vertexIndex < 0) {
                    return getVertexValue(accessor, complements, complements[static_cast<size_t>(~vertexIndex)]);
                }

                return accessor[vertexIndex];
            }

            T operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
                T v0{};
                if (vertex.first < 0) {
                    v0 = getVertexValue(accessor, complements, complements[static_cast<size_t>(~vertex.first)]);
                }
                else {
                    v0 = accessor[vertex.first];
                }

                T v1{};
                if (vertex.second < 0) {
                    v1 = getVertexValue(accessor, complements, complements[static_cast<size_t>(~vertex.second)]);
                }
                else {
                    v1 = accessor[vertex.second];
                }

                return glm::mix(v0, v1, vertex.t);
            }
        };

        return std::visit(Operation { accessor, complements }, vertex);
    }

    template <class TIndex>
    static void upsamplePrimitiveForRasterOverlays(
        const Model& parentModel,
        Model& model,
        Mesh& /*mesh*/,
        MeshPrimitive& primitive,
        CesiumGeometry::UpsampledQuadtreeNode childID
    ) {
        // Add up the per-vertex size of all attributes and create buffers, bufferViews, and accessors
        std::vector<FloatVertexAttribute> attributes;
        attributes.reserve(primitive.attributes.size());

        size_t vertexBufferIndex = model.buffers.size();
        model.buffers.emplace_back();

        size_t indexBufferIndex = model.buffers.size();
        model.buffers.emplace_back();

        size_t vertexBufferViewIndex = model.bufferViews.size();
        model.bufferViews.emplace_back();

        size_t indexBufferViewIndex = model.bufferViews.size();
        model.bufferViews.emplace_back();

        BufferView& vertexBufferView = model.bufferViews[vertexBufferViewIndex];
        vertexBufferView.buffer = static_cast<int>(vertexBufferIndex);
        vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

        BufferView& indexBufferView = model.bufferViews[indexBufferViewIndex];
        indexBufferView.buffer = static_cast<int>(indexBufferIndex);
        indexBufferView.target = BufferView::Target::ARRAY_BUFFER;

        int64_t vertexSizeFloats = 0;
        int32_t uvAccessorIndex = -1;
        int32_t positionAttributeIndex = -1;

        std::vector<std::string> toRemove;

        for (std::pair<const std::string, int>& attribute : primitive.attributes) {
            if (attribute.first == "_CESIUMOVERLAY_0") {
                uvAccessorIndex = attribute.second;

                // Do not include _CESIUMOVERLAY_0, it will be generated later.
                toRemove.push_back(attribute.first);
                continue;
            }

            if (attribute.first.find("_CESIUMOVERLAY_") == 0) {
                // Do not include _CESIUMOVERLAY_*, it will be generated later.
                toRemove.push_back(attribute.first);
                continue;
            }

            if (attribute.second < 0 || attribute.second >= static_cast<int>(parentModel.accessors.size())) {
                toRemove.push_back(attribute.first);
                continue;
            }

            const Accessor& accessor = parentModel.accessors[static_cast<size_t>(attribute.second)];
            if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(parentModel.bufferViews.size())) {
                toRemove.push_back(attribute.first);
                continue;
            }

            const BufferView& bufferView = parentModel.bufferViews[static_cast<size_t>(accessor.bufferView)];
            if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(parentModel.buffers.size())) {
                toRemove.push_back(attribute.first);
                continue;
            }

            const Buffer& buffer = parentModel.buffers[static_cast<size_t>(bufferView.buffer)];

            int64_t accessorByteStride = accessor.computeByteStride(parentModel);
			int64_t accessorComponentElements = accessor.computeNumberOfComponents();
            if (accessor.componentType != Accessor::ComponentType::FLOAT) {
                // Can only interpolate floating point vertex attributes
                return;
            }

            attribute.second = static_cast<int>(model.accessors.size());
            model.accessors.emplace_back();
            Accessor& newAccessor = model.accessors.back();
            newAccessor.bufferView = static_cast<int>(vertexBufferIndex);
            newAccessor.byteOffset = vertexSizeFloats * int64_t(sizeof(float));
            newAccessor.componentType = Accessor::ComponentType::FLOAT;
            newAccessor.type = accessor.type;

            vertexSizeFloats += accessorComponentElements;

            attributes.push_back(FloatVertexAttribute {
                buffer.cesium.data,
                accessor.byteOffset,
                accessorByteStride,
                accessorComponentElements,
                attribute.second,
                std::vector<double>(static_cast<size_t>(accessorComponentElements), std::numeric_limits<double>::max()),
                std::vector<double>(static_cast<size_t>(accessorComponentElements), std::numeric_limits<double>::lowest()),
            });

            // get position to be used to create for skirts later
            if (attribute.first == "POSITION") {
                positionAttributeIndex = int32_t(attributes.size() - 1);
            }
        }

        if (uvAccessorIndex == -1) {
            // We don't know how to divide this primitive, so just copy it verbatim.
            // TODO
            return;
        }

        for (std::string& attribute : toRemove) {
            primitive.attributes.erase(attribute);
        }

        bool keepAboveU = !isWestChild(childID);
        bool keepAboveV = !isSouthChild(childID);

        AccessorView<glm::vec2> uvView(parentModel, uvAccessorIndex);
        AccessorView<TIndex> indicesView(parentModel, primitive.indices);

        if (uvView.status() != AccessorViewStatus::Valid || indicesView.status() != AccessorViewStatus::Valid) {
            return;
        }

        // check if the primitive has skirts
        int64_t indicesBegin = 0;
        int64_t indicesCount = indicesView.size();
        std::optional<SkirtMeshMetadata> parentSkirtMeshMetadata = SkirtMeshMetadata::parseFromGltfExtras(primitive.extras);
        bool hasSkirt = (parentSkirtMeshMetadata != std::nullopt) && (positionAttributeIndex != -1);
        if (hasSkirt) {
            indicesBegin = parentSkirtMeshMetadata->noSkirtIndicesBegin;
            indicesCount = parentSkirtMeshMetadata->noSkirtIndicesCount;
        }

        std::vector<uint32_t> clipVertexToIndices;
        std::vector<CesiumGeometry::TriangleClipVertex> clippedA;
        std::vector<CesiumGeometry::TriangleClipVertex> clippedB;

        // Maps old (parentModel) vertex indices to new (model) vertex indices.
        std::vector<uint32_t> vertexMap(size_t(uvView.size()), std::numeric_limits<uint32_t>::max());

        // std::vector<unsigned char> newVertexBuffer(vertexSizeFloats * sizeof(float));
        // gsl::span<float> newVertexFloats(reinterpret_cast<float*>(newVertexBuffer.data()), newVertexBuffer.size() / sizeof(float));
        std::vector<float> newVertexFloats;
        std::vector<uint32_t> indices;
        EdgeIndices edgeIndices;

        for (int64_t i = indicesBegin; i < indicesBegin + indicesCount; i += 3) {
            TIndex i0 = indicesView[i];
            TIndex i1 = indicesView[i + 1];
            TIndex i2 = indicesView[i + 2];

            glm::vec2 uv0 = uvView[i0];
            glm::vec2 uv1 = uvView[i1];
            glm::vec2 uv2 = uvView[i2];

            // Clip this triangle against the East-West boundary
            clippedA.clear();
            clipTriangleAtAxisAlignedThreshold(0.5, keepAboveU, static_cast<int>(i0), static_cast<int>(i1), static_cast<int>(i2), uv0.x, uv1.x, uv2.x, clippedA);

            if (clippedA.size() < 3) {
                // No part of this triangle is inside the target tile.
                continue;
            }

            // Clip the first clipped triange against the North-South boundary
            clipVertexToIndices.clear();
            clippedB.clear();
            clipTriangleAtAxisAlignedThreshold(
                0.5,
                keepAboveV,
                ~0,
                ~1,
                ~2,
                getVertexValue(uvView, clippedA[0]).y,
                getVertexValue(uvView, clippedA[1]).y,
                getVertexValue(uvView, clippedA[2]).y,
                clippedB
            );

            // Add the clipped triangle or quad, if any
            addClippedPolygon(newVertexFloats, indices, attributes, vertexMap, clipVertexToIndices, clippedA, clippedB);
            if (hasSkirt) {
                addEdge(edgeIndices, 0.5, 0.5, keepAboveU, keepAboveV, uvView, clipVertexToIndices, clippedA, clippedB);
            }

            // If the East-West clip yielded a quad (rather than a triangle), clip the second triangle of the quad, too.
            if (clippedA.size() > 3) {
                clipVertexToIndices.clear();
                clippedB.clear();
                clipTriangleAtAxisAlignedThreshold(
                    0.5,
                    keepAboveV,
                    ~0,
                    ~2,
                    ~3,
                    getVertexValue(uvView, clippedA[0]).y,
                    getVertexValue(uvView, clippedA[2]).y,
                    getVertexValue(uvView, clippedA[3]).y,
                    clippedB
                );

                // Add the clipped triangle or quad, if any
                addClippedPolygon(newVertexFloats, indices, attributes, vertexMap, clipVertexToIndices, clippedA, clippedB);
                if (hasSkirt) {
                    addEdge(edgeIndices, 0.5, 0.5, keepAboveU, keepAboveV, uvView, clipVertexToIndices, clippedA, clippedB);
                }
            }
        }

        // create mesh with skirt
        std::optional<SkirtMeshMetadata> skirtMeshMetadata;
        if (hasSkirt) {
            skirtMeshMetadata = std::make_optional<SkirtMeshMetadata>();
            skirtMeshMetadata->noSkirtIndicesBegin = 0;
            skirtMeshMetadata->noSkirtIndicesCount = static_cast<uint32_t>(indices.size());
            skirtMeshMetadata->meshCenter = parentSkirtMeshMetadata->meshCenter;
            addSkirts(newVertexFloats, 
                indices, 
                attributes,
                childID,
                *skirtMeshMetadata,
                *parentSkirtMeshMetadata,
                edgeIndices, 
                vertexSizeFloats, 
                positionAttributeIndex);
        }

        // Update the accessor vertex counts and min/max values
        int64_t numberOfVertices = int64_t(newVertexFloats.size()) / vertexSizeFloats;
        for (const FloatVertexAttribute& attribute : attributes) {
            Accessor& accessor = model.accessors[static_cast<size_t>(attribute.accessorIndex)];
            accessor.count = numberOfVertices;
            accessor.min = std::move(attribute.minimums);
            accessor.max = std::move(attribute.maximums);
        }

        // Add an accessor for the indices
        size_t indexAccessorIndex = model.accessors.size();
        model.accessors.emplace_back();
        Accessor& newIndicesAccessor = model.accessors.back();
        newIndicesAccessor.bufferView = static_cast<int>(indexBufferViewIndex);
        newIndicesAccessor.byteOffset = 0;
        newIndicesAccessor.count = int64_t(indices.size());
        newIndicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
        newIndicesAccessor.type = Accessor::Type::SCALAR;

        // Populate the buffers
        Buffer& vertexBuffer = model.buffers[vertexBufferIndex];
        vertexBuffer.cesium.data.resize(newVertexFloats.size() * sizeof(float));
        float* pAsFloats = reinterpret_cast<float*>(vertexBuffer.cesium.data.data());
        std::copy(newVertexFloats.begin(), newVertexFloats.end(), pAsFloats);
        vertexBufferView.byteLength = int64_t(vertexBuffer.cesium.data.size());
        vertexBufferView.byteStride = vertexSizeFloats * int64_t(sizeof(float));

        Buffer& indexBuffer = model.buffers[indexBufferIndex];
        indexBuffer.cesium.data.resize(indices.size() * sizeof(uint32_t));
        uint32_t* pAsUint32s = reinterpret_cast<uint32_t*>(indexBuffer.cesium.data.data());
        std::copy(indices.begin(), indices.end(), pAsUint32s);
        indexBufferView.byteLength = int64_t(indexBuffer.cesium.data.size());

        // add skirts to extras to be upsampled later if needed
        if (hasSkirt) {
            primitive.extras = SkirtMeshMetadata::createGltfExtras(*skirtMeshMetadata);
        }

        primitive.indices = static_cast<int>(indexAccessorIndex);
    }

    static uint32_t getOrCreateVertex(
        std::vector<float>& output,
        std::vector<FloatVertexAttribute>& attributes,
        std::vector<uint32_t>& vertexMap,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const CesiumGeometry::TriangleClipVertex& clipVertex
    ) {
        const int* pIndex = std::get_if<int>(&clipVertex);
        if (pIndex) {
            if (*pIndex < 0) {
                return getOrCreateVertex(output, attributes, vertexMap, complements, complements[static_cast<size_t>(~(*pIndex))]);
            }

            uint32_t existingIndex = vertexMap[static_cast<size_t>(*pIndex)];
            if (existingIndex != std::numeric_limits<uint32_t>::max()) {
                return existingIndex;
            }
        }

        uint32_t beforeOutput = static_cast<uint32_t>(output.size());
        copyVertexAttributes(attributes, complements, clipVertex, output);
        uint32_t newIndex = beforeOutput / (static_cast<uint32_t>(output.size()) - beforeOutput);

        if (pIndex && *pIndex >= 0) {
            vertexMap[static_cast<size_t>(*pIndex)] = newIndex;
        }

        return newIndex;
    }

    static void addClippedPolygon(
        std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        std::vector<uint32_t>& vertexMap,
        std::vector<uint32_t>& clipVertexToIndices,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult
    ) {
        if (clipResult.size() < 3) {
            return;
        }

        uint32_t i0 = getOrCreateVertex(output, attributes, vertexMap, complements, clipResult[0]);
        uint32_t i1 = getOrCreateVertex(output, attributes, vertexMap, complements, clipResult[1]);
        uint32_t i2 = getOrCreateVertex(output, attributes, vertexMap, complements, clipResult[2]);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        clipVertexToIndices.push_back(i0);
        clipVertexToIndices.push_back(i1);
        clipVertexToIndices.push_back(i2);

        if (clipResult.size() > 3) {
            uint32_t i3 = getOrCreateVertex(output, attributes, vertexMap, complements, clipResult[3]);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);

            clipVertexToIndices.push_back(i3);
        }
    }

    static void addEdge(EdgeIndices& edgeIndices,
        double thresholdU,
        double thresholdV,
        bool keepAboveU,
        bool keepAboveV,
        const AccessorView<glm::vec2>& uvs,
        const std::vector<uint32_t>& clipVertexToIndices,
        const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
        const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult)
    {
        for (uint32_t i = 0; i < clipVertexToIndices.size(); ++i) {
            glm::vec2 uv = getVertexValue(uvs, complements, clipResult[i]);

            if (CesiumUtility::Math::equalsEpsilon(uv.x, 0.0, CesiumUtility::Math::EPSILON4)) {
                edgeIndices.west.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
            }

            if (CesiumUtility::Math::equalsEpsilon(uv.x, 1.0, CesiumUtility::Math::EPSILON4)) {
                edgeIndices.east.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
            }

            if (CesiumUtility::Math::equalsEpsilon(uv.x, thresholdU, CesiumUtility::Math::EPSILON4)) {
                if (keepAboveU) {
                    edgeIndices.west.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
                }
                else {
                    edgeIndices.east.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
                }
            }

            if (CesiumUtility::Math::equalsEpsilon(uv.y, 0.0, CesiumUtility::Math::EPSILON4)) {
                edgeIndices.south.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
            }

            if (CesiumUtility::Math::equalsEpsilon(uv.y, 1.0, CesiumUtility::Math::EPSILON4)) {
                edgeIndices.north.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
            }

            if (CesiumUtility::Math::equalsEpsilon(uv.y, thresholdV, CesiumUtility::Math::EPSILON4)) {
                if (keepAboveV) {
                    edgeIndices.south.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
                }
                else {
                    edgeIndices.north.emplace_back(EdgeVertex{ clipVertexToIndices[i], uv });
                }
            }
        }
    }

    static void addSkirt(std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        const std::vector<uint32_t>& edgeIndices,
        const glm::dvec3& center,
        double skirtHeight,
        int64_t vertexSizeFloats,
        int32_t positionAttributeIndex) 
    {
        const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

        uint32_t newEdgeIndex = uint32_t(output.size() / size_t(vertexSizeFloats));
        for (size_t i = 0; i < edgeIndices.size(); ++i) {
            uint32_t edgeIdx = edgeIndices[i];
            uint32_t offset = 0;
            for (size_t j = 0; j < attributes.size(); ++j) {
                FloatVertexAttribute& attribute = attributes[j];
                uint32_t valueIndex = offset + uint32_t(vertexSizeFloats) * edgeIdx;

                if (int32_t(j) == positionAttributeIndex) {
                    glm::dvec3 position{ output[valueIndex], output[valueIndex + 1], output[valueIndex + 2] };
                    position += center;

                    position -= skirtHeight * ellipsoid.geodeticSurfaceNormal(position);
                    position -= center;

                    for (uint32_t c = 0; c < 3; ++c) {
                        output.push_back(static_cast<float>(position[static_cast<int32_t>(c)]));
                        attribute.minimums[c] = glm::min(attribute.minimums[c], position[static_cast<int32_t>(c)]);
                        attribute.maximums[c] = glm::max(attribute.maximums[c], position[static_cast<int32_t>(c)]);
                    }
                }
                else {
                    for (uint32_t c = 0; c < static_cast<uint32_t>(attribute.numberOfFloatsPerVertex); ++c) {
                        output.push_back(output[valueIndex + c]);
                        attribute.minimums[c] = glm::min(attribute.minimums[c], static_cast<double>(output.back()));
                        attribute.maximums[c] = glm::max(attribute.maximums[c], static_cast<double>(output.back()));
                    }
                }

                offset += static_cast<uint32_t>(attribute.numberOfFloatsPerVertex);
            }

            if (i < edgeIndices.size() - 1) {
                uint32_t nextEdgeIdx = edgeIndices[i + 1];
                indices.push_back(edgeIdx);
                indices.push_back(nextEdgeIdx);
                indices.push_back(newEdgeIndex);

                indices.push_back(newEdgeIndex);
                indices.push_back(nextEdgeIdx);
                indices.push_back(newEdgeIndex + 1);
            }

            ++newEdgeIndex;
        }
    }

    static void addSkirts(std::vector<float>& output,
        std::vector<uint32_t>& indices,
        std::vector<FloatVertexAttribute>& attributes,
        CesiumGeometry::UpsampledQuadtreeNode childID,
        SkirtMeshMetadata &currentSkirt,
        const SkirtMeshMetadata &parentSkirt,
        EdgeIndices &edgeIndices,
        int64_t vertexSizeFloats,
        int32_t positionAttributeIndex) 
    {
        glm::dvec3 center = currentSkirt.meshCenter;
        double shortestSkirtHeight = glm::min(parentSkirt.skirtWestHeight, parentSkirt.skirtEastHeight);
        shortestSkirtHeight = glm::min(shortestSkirtHeight, parentSkirt.skirtSouthHeight);
        shortestSkirtHeight = glm::min(shortestSkirtHeight, parentSkirt.skirtNorthHeight);

        // west
        if (isWestChild(childID)) {
            currentSkirt.skirtWestHeight = parentSkirt.skirtWestHeight;
        }
        else {
            currentSkirt.skirtWestHeight = shortestSkirtHeight * 0.5;
        }

        std::vector<uint32_t> sortEdgeIndices(edgeIndices.west.size());
        std::sort(edgeIndices.west.begin(), 
            edgeIndices.west.end(), 
            [](const EdgeVertex &lhs, const EdgeVertex &rhs) {
                return lhs.uv.y < rhs.uv.y;
            });
        std::transform(edgeIndices.west.begin(),
            edgeIndices.west.end(),
            sortEdgeIndices.begin(),
            [](const EdgeVertex& v) { return v.index; });
        addSkirt(output, 
            indices, 
            attributes,
            sortEdgeIndices, 
            center, 
            currentSkirt.skirtWestHeight, 
            vertexSizeFloats, 
            positionAttributeIndex);

        // south
        if (isSouthChild(childID)) {
            currentSkirt.skirtSouthHeight = parentSkirt.skirtSouthHeight;
        }
        else {
            currentSkirt.skirtSouthHeight = shortestSkirtHeight * 0.5;
        }

        sortEdgeIndices.resize(edgeIndices.south.size());
        std::sort(edgeIndices.south.begin(), 
            edgeIndices.south.end(), 
            [](const EdgeVertex &lhs, const EdgeVertex &rhs) {
                return lhs.uv.x > rhs.uv.x;
            });
        std::transform(edgeIndices.south.begin(),
            edgeIndices.south.end(),
            sortEdgeIndices.begin(),
            [](const EdgeVertex& v) { return v.index; });
        addSkirt(output, 
            indices, 
            attributes,
            sortEdgeIndices, 
            center, 
            currentSkirt.skirtSouthHeight, 
            vertexSizeFloats, 
            positionAttributeIndex);

        // east
        if (!isWestChild(childID)) {
            currentSkirt.skirtEastHeight = parentSkirt.skirtEastHeight;
        }
        else {
            currentSkirt.skirtEastHeight = shortestSkirtHeight * 0.5;
        }

        sortEdgeIndices.resize(edgeIndices.east.size());
        std::sort(edgeIndices.east.begin(), 
            edgeIndices.east.end(), 
            [](const EdgeVertex &lhs, const EdgeVertex &rhs) {
                return lhs.uv.y > rhs.uv.y;
            });
        std::transform(edgeIndices.east.begin(),
            edgeIndices.east.end(),
            sortEdgeIndices.begin(),
            [](const EdgeVertex& v) { return v.index; });
        addSkirt(output, 
            indices, 
            attributes,
            sortEdgeIndices, 
            center, 
            currentSkirt.skirtEastHeight, 
            vertexSizeFloats, 
            positionAttributeIndex);

        // north
        if (!isSouthChild(childID)) {
            currentSkirt.skirtNorthHeight = parentSkirt.skirtNorthHeight;
        }
        else {
            currentSkirt.skirtNorthHeight = shortestSkirtHeight * 0.5;
        }

        sortEdgeIndices.resize(edgeIndices.north.size());
        std::sort(edgeIndices.north.begin(), 
            edgeIndices.north.end(), 
            [](const EdgeVertex &lhs, const EdgeVertex &rhs) {
                return lhs.uv.x < rhs.uv.x;
            });
        std::transform(edgeIndices.north.begin(),
            edgeIndices.north.end(),
            sortEdgeIndices.begin(),
            [](const EdgeVertex& v) { return v.index; });
        addSkirt(output, 
            indices, 
            attributes,
            sortEdgeIndices, 
            center, 
            currentSkirt.skirtNorthHeight, 
            vertexSizeFloats, 
            positionAttributeIndex);
    }

    static void upsamplePrimitiveForRasterOverlays(
        const Model& parentModel,
        Model& model,
        Mesh& mesh,
        MeshPrimitive& primitive,
        CesiumGeometry::UpsampledQuadtreeNode childID
    ) {
        if (
            primitive.mode != MeshPrimitive::Mode::TRIANGLES ||
            primitive.indices < 0 ||
            primitive.indices >= static_cast<int>(parentModel.accessors.size())
        ) {
            // Not indexed triangles, so we don't know how to divide this primitive (yet).
            // So just copy it verbatim.
            // TODO
            return;
        }

        const Accessor& indicesAccessorGltf = parentModel.accessors[static_cast<size_t>(primitive.indices)];
        if (indicesAccessorGltf.componentType == Accessor::ComponentType::UNSIGNED_SHORT) {
            upsamplePrimitiveForRasterOverlays<uint16_t>(parentModel, model, mesh, primitive, childID);
        } else if (indicesAccessorGltf.componentType == Accessor::ComponentType::UNSIGNED_INT) {
            upsamplePrimitiveForRasterOverlays<uint32_t>(parentModel, model, mesh, primitive, childID);
        }
    }

}
