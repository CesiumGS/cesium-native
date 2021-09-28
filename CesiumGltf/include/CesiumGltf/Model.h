#pragma once

#include "CesiumGltf/Library.h"
#include "CesiumGltf/ModelSpec.h"

#include <glm/mat4x4.hpp>

#include <functional>

namespace CesiumGltf {

/** @copydoc ModelSpec */
struct CESIUMGLTF_API Model : public ModelSpec {
  /**
   * @brief Merges another model into this one.
   *
   * After this method returns, this `Model` contains all of the
   * elements that were originally in it _plus_ all of the elements
   * that were in `rhs`. Element indices are updated accordingly.
   * However, element indices in {@link ExtensibleObject::extras}, if any,
   * are _not_ updated.
   *
   * @param rhs The model to merge into this one.
   */
  void merge(Model&& rhs);

  /**
   * @brief A callback function for {@link forEachPrimitiveInScene}.
   */
  typedef void ForEachPrimitiveInSceneCallback(
      Model& gltf,
      Node& node,
      Mesh& mesh,
      MeshPrimitive& primitive,
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
   * @param sceneID The scene ID (index)
   * @param callback The callback to apply
   */
  void forEachPrimitiveInScene(
      int sceneID,
      std::function<ForEachPrimitiveInSceneCallback>&& callback);

  /**
   * @brief A callback function for {@link forEachPrimitiveInScene}.
   */
  typedef void ForEachPrimitiveInSceneConstCallback(
      const Model& gltf,
      const Node& node,
      const Mesh& mesh,
      const MeshPrimitive& primitive,
      const glm::dmat4& transform);

  /** @copydoc Gltf::forEachPrimitiveInScene() */
  void forEachPrimitiveInScene(
      int sceneID,
      std::function<ForEachPrimitiveInSceneConstCallback>&& callback) const;

  /**
   * @brief Fills in smooth normals for any primitives with missing normals.
   */
  void generateMissingNormalsSmooth();

  /**
   * @brief Safely gets the element with a given index, returning a default
   * instance if the index is outside the range.
   *
   * @tparam T The type of the array.
   * @param items The array.
   * @param index The index of the array element to retrieve.
   * @return The requested element, or a default instance if the index is
   * invalid.
   */
  template <typename T>
  static const T& getSafe(const std::vector<T>& items, int32_t index) {
    static T defaultObject;
    if (index < 0 || static_cast<size_t>(index) >= items.size()) {
      return defaultObject;
    } else {
      return items[static_cast<size_t>(index)];
    }
  }

  /**
   * @brief Safely gets a pointer to the element with a given index, returning
   * `nullptr` if the index is outside the range.
   *
   * @tparam T The type of the array.
   * @param pItems The array.
   * @param index The index of the array element to retrieve.
   * @return A pointer to the requested element, or `nullptr` if the index is
   * invalid.
   */
  template <typename T>
  static const T*
  getSafe(const std::vector<T>* pItems, int32_t index) noexcept {
    if (index < 0 || static_cast<size_t>(index) >= pItems->size()) {
      return nullptr;
    } else {
      return &(*pItems)[static_cast<size_t>(index)];
    }
  }

  /**
   * @brief Safely gets a pointer to the element with a given index, returning
   * `nullptr` if the index is outside the range.
   *
   * @tparam T The type of the array.
   * @param pItems The array.
   * @param index The index of the array element to retrieve.
   * @return A pointer to the requested element, or `nullptr` if the index is
   * invalid.
   */
  template <typename T>
  static T* getSafe(std::vector<T>* pItems, int32_t index) noexcept {
    if (index < 0 || static_cast<size_t>(index) >= pItems->size()) {
      return nullptr;
    } else {
      return &(*pItems)[static_cast<size_t>(index)];
    }
  }
};
} // namespace CesiumGltf
