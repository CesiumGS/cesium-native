#pragma once

#include "Cesium3DTiles/Library.h"
#include "CesiumGltf/Model.h"
#include <functional>
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <optional>

namespace Cesium3DTiles {

/**
 * @brief Functions for loading and processing glTF models.
 *
 * This class offers basic functions for loading glTF models as
 * `CesiumGltf::Model` instances, and for processing the mesh primitives that
 * appear in the resulting models.
 */
class CESIUM3DTILES_API Gltf final {

public:
  /** @brief This class cannot be instantiated */
  Gltf() = delete;

  /**
   * @brief A callback function for {@link Gltf::forEachPrimitiveInScene}.
   */
  typedef void ForEachPrimitiveInSceneCallback(
      CesiumGltf::Model& gltf,
      CesiumGltf::Node& node,
      CesiumGltf::Mesh& mesh,
      CesiumGltf::MeshPrimitive& primitive,
      const glm::dmat4& transform);

  /**
   * @brief Apply the given callback to all relevant primitives.
   *
   * If the given `sceneID` is non-negative and exists in the given glTF,
   * then the given callback will be applied to all meshes of this scene.
   *
   * If the given `sceneId` is negative, then the meshes that the callback
   * will be applied to depends on the structure of the glTF model:
   *
   * * If the glTF model has a default scene, then it will
   *   be applied to all meshes of the default scene.
   * * Otherwise, it will be applied to all meshes of the the first scene.
   * * Otherwise (if the glTF model does not contain any scenes), it will
   *   be applied to all meshes that can be found by starting a traversal
   *   at the root node.
   * * Otherwise (if there are no scenes and no nodes), then all meshes
   *   will be traversed.
   *
   * @param gltf The glTF model.
   * @param sceneID The scene ID (index)
   * @param callback The callback to apply
   */
  static void forEachPrimitiveInScene(
      CesiumGltf::Model& gltf,
      int sceneID,
      std::function<ForEachPrimitiveInSceneCallback>&& callback);

  /**
   * @brief A callback function for {@link Gltf::forEachPrimitiveInScene}.
   */
  typedef void ForEachPrimitiveInSceneConstCallback(
      const CesiumGltf::Model& gltf,
      const CesiumGltf::Node& node,
      const CesiumGltf::Mesh& mesh,
      const CesiumGltf::MeshPrimitive& primitive,
      const glm::dmat4& transform);

  /** @copydoc Gltf::forEachPrimitiveInScene() */
  static void forEachPrimitiveInScene(
      const CesiumGltf::Model& gltf,
      int sceneID,
      std::function<ForEachPrimitiveInSceneConstCallback>&& callback);
};

} // namespace Cesium3DTiles
