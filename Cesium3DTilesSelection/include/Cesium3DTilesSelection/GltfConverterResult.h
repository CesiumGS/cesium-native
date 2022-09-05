#pragma once

#include "Library.h"

#include <Cesium3DTilesSelection/ErrorList.h>
#include <CesiumGltf/Model.h>

#include <optional>

namespace Cesium3DTilesSelection {
/**
 * @brief The result of converting a binary content to gltf model.
 *
 * Instances of this structure are created internally, by the
 * {@link GltfConverters}, when the response to a network request for
 * loading the tile content was received.
 */
struct CESIUM3DTILESSELECTION_API GltfConverterResult {
  /**
   * @brief The gltf model converted from a binary content. This is empty if
   * there are errors during the conversion
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief The error and warning list when converting a binary content to gltf
   * model. This is empty if there are no errors during the conversion
   */
  ErrorList errors;
};
} // namespace Cesium3DTilesSelection
