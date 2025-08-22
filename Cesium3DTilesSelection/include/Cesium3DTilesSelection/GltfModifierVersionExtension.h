#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <optional>

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTilesSelection {

/** @brief An extension holding the "version" of a glTF produced by
 * {@link GltfModifier}.
 */
struct CESIUM3DTILESSELECTION_API GltfModifierVersionExtension
    : public CesiumUtility::ExtensibleObject {
  /**
   * @brief Gets the version number of the given model, or `std::nullopt` if
   * the model does not have the `GltfModifierVersionExtension` attached to it
   * yet.
   */
  static std::optional<int64_t>
  getVersion(const CesiumGltf::Model& model) noexcept;

  /**
   * @brief Sets the version number of the given model to the given value.
   *
   * This method creates the `GltfModifierVersionExtension` and attaches it to
   * the `Model` if it does not already exist. If it does exist, the
   * version number is updated.
   */
  static void setVersion(CesiumGltf::Model& model, int64_t version) noexcept;

  /**
   * @brief The original name of this type.
   */
  static constexpr const char* TypeName = "GltfModifierVersionExtension";
  /**
   * @brief The official name of the extension. This should be the same as its
   * key in the `extensions` object.
   * */
  static constexpr const char* ExtensionName =
      "CESIUM_INTERNAL_gltf_modifier_version";

  /**
   * @brief The current {@link GltfModifier} version number of the model.
   */
  int64_t version = 0;

  /**
   * @brief Calculates the size in bytes of this object, including the contents
   * of all collections, pointers, and strings. This will NOT include the size
   * of any extensions attached to the object. Calling this method may be slow
   * as it requires traversing the object's entire structure.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(GltfModifierVersionExtension));
    accum += CesiumUtility::ExtensibleObject::getSizeBytes() -
             int64_t(sizeof(CesiumUtility::ExtensibleObject));
    return accum;
  }
};

} // namespace Cesium3DTilesSelection
