#pragma once

#include <CesiumGltf/Library.h>
#include <CesiumGltf/ModelSpec.h>
#include <CesiumUtility/ErrorList.h>

#include <glm/mat4x4.hpp>

#include <functional>

namespace CesiumGltf {

/** @copydoc ModelSpec */
struct CESIUMGLTF_API Model : public ModelSpec {
  Model() = default;

  /**
   * @brief Merges another model into this one.
   *
   * After this method returns, this `Model` contains all of the
   * elements that were originally in it _plus_ all of the elements
   * that were in `rhs`. Element indices are updated accordingly.
   * However, element indices in {@link CesiumUtility::ExtensibleObject::extras}, if any,
   * are _not_ updated.
   *
   * @param rhs The model to merge into this one.
   */
  CesiumUtility::ErrorList merge(Model&& rhs);

  /**
   * @brief A callback function for {@link forEachRootNodeInScene}.
   */
  typedef void ForEachRootNodeInSceneCallback(Model& gltf, Node& node);

  /**
   * @brief A callback function for {@link forEachRootNodeInScene}.
   */
  typedef void
  ForEachRootNodeInSceneConstCallback(const Model& gltf, const Node& node);

  /**
   * @brief Apply the given callback to the root nodes of the scene.
   *
   * If the given `sceneID` is non-negative and exists in the given glTF,
   * then the given callback will be applied to all root nodes of this scene.
   *
   * If the given `sceneId` is negative, then the nodes that the callback
   * will be applied to depends on the structure of the glTF model:
   *
   * * If the glTF model has a default scene, then it will
   *   be applied to all root nodes of the default scene.
   * * Otherwise, it will be applied to all root nodes of the first scene.
   * * Otherwise (if the glTF model does not contain any scenes), it will
   *   be applied to the first node.
   * * Otherwise (if there are no scenes and no nodes), then this method will do
   *   nothing.
   *
   * @param sceneID The scene ID (index)
   * @param callback The callback to apply
   */
  void forEachRootNodeInScene(
      int32_t sceneID,
      std::function<ForEachRootNodeInSceneCallback>&& callback);

  /** @copydoc Model::forEachRootNodeInScene */
  void forEachRootNodeInScene(
      int32_t sceneID,
      std::function<ForEachRootNodeInSceneConstCallback>&& callback) const;

  /**
   * @brief A callback function for {@link forEachNodeInScene}.
   */
  typedef void ForEachNodeInSceneCallback(
      Model& gltf,
      Node& node,
      const glm::dmat4& transform);

  /**
   * @brief Apply the given callback to all nodes in the scene.
   *
   * If the given `sceneID` is non-negative and exists in the given glTF,
   * then the given callback will be applied to all nodes in this scene.
   *
   * If the given `sceneId` is negative, then the nodes that the callback
   * will be applied to depends on the structure of the glTF model:
   *
   * * If the glTF model has a default scene, then it will
   *   be applied to all nodes in the default scene.
   * * Otherwise, it will be applied to all nodes in the first scene.
   * * Otherwise (if the glTF model does not contain any scenes), it will
   *   be applied to the first node.
   * * Otherwise (if there are no scenes and no nodes), then this method will do
   *   nothing.
   *
   * @param sceneID The scene ID (index)
   * @param callback The callback to apply
   */
  void forEachNodeInScene(
      int32_t sceneID,
      std::function<ForEachNodeInSceneCallback>&& callback);

  /**
   * @brief A callback function for {@link forEachNodeInScene}.
   */
  typedef void ForEachNodeInSceneConstCallback(
      const Model& gltf,
      const Node& node,
      const glm::dmat4& transform);

  /** @copydoc Model::forEachNodeInScene */
  void forEachNodeInScene(
      int32_t sceneID,
      std::function<ForEachNodeInSceneConstCallback>&& callback) const;

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
      int32_t sceneID,
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

  /** @copydoc Model::forEachPrimitiveInScene */
  void forEachPrimitiveInScene(
      int32_t sceneID,
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
    if (index < 0 || size_t(index) >= items.size()) {
      return defaultObject;
    } else {
      return items[size_t(index)];
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
    if (index < 0 || size_t(index) >= pItems->size()) {
      return nullptr;
    } else {
      return &(*pItems)[size_t(index)];
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
    if (index < 0 || size_t(index) >= pItems->size()) {
      return nullptr;
    } else {
      return &(*pItems)[size_t(index)];
    }
  }

  /**
   * @brief Adds an extension to the {@link ModelSpec::extensionsUsed}
   * property, if it is not already present.
   *
   * @param extensionName The name of the used extension.
   */
  void addExtensionUsed(const std::string& extensionName);

  /**
   * @brief Adds an extension to the {@link ModelSpec::extensionsRequired}
   * property, if it is not already present.
   *
   * Calling this function also adds the extension to `extensionsUsed`, if it's
   * not already present.
   *
   * @param extensionName The name of the required extension.
   */
  void addExtensionRequired(const std::string& extensionName);

  /**
   * @brief Removes an extension from the {@link ModelSpec::extensionsUsed}
   * property.
   *
   * @param extensionName The name of the used extension.
   */
  void removeExtensionUsed(const std::string& extensionName);

  /**
   * @brief Removes an extension from the {@link ModelSpec::extensionsRequired}
   * property.
   *
   * Calling this function also removes the extension from `extensionsUsed`.
   *
   * @param extensionName The name of the required extension.
   */
  void removeExtensionRequired(const std::string& extensionName);

  /**
   * @brief Determines whether a given extension name is listed in the model's
   * {@link ModelSpec::extensionsUsed} property.
   *
   * @param extensionName The extension name to check.
   * @returns True if the extension is found in `extensionsUsed`; otherwise,
   * false.
   */
  bool isExtensionUsed(const std::string& extensionName) const noexcept;

  /**
   * @brief Determines whether a given extension name is listed in the model's
   * {@link ModelSpec::extensionsRequired} property.
   *
   * @param extensionName The extension name to check.
   * @returns True if the extension is found in `extensionsRequired`; otherwise,
   * false.
   */
  bool isExtensionRequired(const std::string& extensionName) const noexcept;
};

} // namespace CesiumGltf
