#include "Cesium3DTiles/GltfAccessor.h"
#include "CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h"
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

    template <class TIndex>
    static void upsamplePrimitiveForRasterOverlays(
        const tinygltf::Model& parentModel,
        tinygltf::Model& model,
        tinygltf::Mesh& mesh,
        tinygltf::Primitive& primitive,
        CesiumGeometry::QuadtreeChild childID
    ) {
        auto uvIt = primitive.attributes.find("_CESIUMOVERLAY_0");
        if (uvIt == primitive.attributes.end()) {
            // We don't know how to divide this primitive, so just copy it verbatim.
            // TODO
            return;
        }

        bool keepAboveU = childID == CesiumGeometry::QuadtreeChild::LowerRight || childID == CesiumGeometry::QuadtreeChild::UpperRight;
        bool keepAboveV = childID == CesiumGeometry::QuadtreeChild::UpperLeft || childID == CesiumGeometry::QuadtreeChild::UpperRight;

        int uvAccessorIndex = uvIt->second;
        GltfAccessor<glm::vec2> uvAccessor(parentModel, uvAccessorIndex);
        GltfAccessor<TIndex> indicesAccessor(parentModel, primitive.indices);

        std::vector<TriangleClipVertex> clipped;

        for (size_t i = 0; i < indicesAccessor.size(); i += 3) {
            TIndex i0 = indicesAccessor[i];
            TIndex i1 = indicesAccessor[i + 1];
            TIndex i2 = indicesAccessor[i + 2];

            glm::vec2 uv0 = uvAccessor[i0];
            glm::vec2 uv1 = uvAccessor[i1];
            glm::vec2 uv2 = uvAccessor[i2];

            // Clip this triangle against the North-South boundary
            clipped.clear();
            clipTriangleAtAxisAlignedThreshold(0.5, keepAboveV, uv0.y, uv1.y, uv2.y, clipped);

            if (clipped.empty()) {
                // No part of this triangle is inside the target tile.
                continue;
            }

            // Get the first clipped triangle
            
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
