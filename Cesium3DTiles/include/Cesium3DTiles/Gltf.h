#pragma once

#include "Cesium3DTiles/Library.h"
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <optional>
#include <tiny_gltf.h>

namespace Cesium3DTiles {

    /**
     * @brief Functions for loading and processing glTF models.
     * 
     * This class offers basic functions for loading glTF models as `tinygltf::Model`
     * instances, and for processing the mesh primitives that appear in the resulting 
     * models.
     */
    class CESIUM3DTILES_API Gltf {
    public:

        /**
         * @brief A summary of the result of loading a glTF model from raw data.
         */
        struct LoadResult {
        public:

            /**
             * @brief The gltf model that was loaded.
             * 
             * This will be `std::nullopt` if the model was not loaded successfully.
             */
            std::optional<tinygltf::Model> model;

            /**
             * @brief Warning messages from the attempt to load the model.
             */
            std::string warnings;

            /**
             * @brief Error messages from the attempt to load the model.
             */
            std::string errors;
        };

        /**
         * @brief Load a glTF model from the given input data.
         * 
         * @param data The input data.
         * @return The {@link LoadResult} containing the model (if it could be loaded),
         * and possible warning- or error messages.
         */
        static LoadResult load(const gsl::span<const uint8_t>& data);

        /**
         * @brief A callback function for {@link Gltf::forEachPrimitiveInScene}.
         */
        typedef void ForEachPrimitiveInSceneCallback(
            tinygltf::Model& gltf,
            tinygltf::Node& node,
            tinygltf::Mesh& mesh,
            tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );

        /**
         * @brief Apply the given callback to all relevant primitives.
         * 
         * The callback will be applied to all meshes, depending on
         * the given `sceneID` and the given glTF asset:
         * 
         * If the given `sceneID` is non-negative and exists in the given glTF,
         * then it will be applied to all meshes of this scene.
         * Otherwise, if the glTF model as a default scene, then it will
         * be applied to all meshes of the default scene. 
         * Otherwise, it will be applied to all meshes of the the first scene.
         * Otherwise, it will be applied to all meshes that can be found
         * by starting a traversal at the root node.
         * Otherwise (if there are no scenes and no nodes), then all meshes 
         * will be traversed.
         * 
         * @param gltf The glTF model.
         * @param sceneID The scene ID (index)
         * @param callback The callback to apply
         */
        static void forEachPrimitiveInScene(tinygltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback);

        /**
         * @brief A callback function for {@link Gltf::forEachPrimitiveInScene}.
         */
        typedef void ForEachPrimitiveInSceneConstCallback(
            const tinygltf::Model& gltf,
            const tinygltf::Node& node,
            const tinygltf::Mesh& mesh,
            const tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );

        /** @copydoc Gltf::forEachPrimitiveInScene() */
        static void forEachPrimitiveInScene(const tinygltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneConstCallback>&& callback);
    };

}
