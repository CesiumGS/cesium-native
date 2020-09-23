#pragma once

#include "Cesium3DTiles/Library.h"
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <optional>
#include <tiny_gltf.h>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Gltf {
    public:
        struct LoadResult {
        public:
            std::optional<tinygltf::Model> model;
            std::string warnings;
            std::string errors;
        };

        static LoadResult load(const gsl::span<const uint8_t>& data);

        typedef void ForEachPrimitiveInSceneCallback(
            tinygltf::Model& gltf,
            tinygltf::Node& node,
            tinygltf::Mesh& mesh,
            tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );

        static void forEachPrimitiveInScene(tinygltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback);

        typedef void ForEachPrimitiveInSceneConstCallback(
            const tinygltf::Model& gltf,
            const tinygltf::Node& node,
            const tinygltf::Mesh& mesh,
            const tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );

        static void forEachPrimitiveInScene(const tinygltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneConstCallback>&& callback);
    };

}
