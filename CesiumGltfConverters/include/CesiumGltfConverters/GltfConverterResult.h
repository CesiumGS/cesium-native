#pragma once

#include <CesiumGltf/Model.h>
#include <CesiumGltfConverters/Library.h>
#include <CesiumUtility/ErrorList.h>

#include <glm/common.hpp>

#include <optional>
#include <string>
#include <vector>

namespace CesiumGltfConverters {
/**
 * @brief The result of converting a binary content to gltf model.
 *
 * Instances of this structure are created internally, by the
 * @ref GltfConverters, when the response to a network request for
 * loading the tile content was received.
 */
struct CESIUMGLTFCONVERTERS_API GltfConverterResult {
  /**
   * @brief The gltf model converted from a binary content. This is empty if
   * there are errors during the conversion
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief The error and warning list when converting a binary content to gltf
   * model. This is empty if there are no errors during the conversion
   */
  CesiumUtility::ErrorList errors;
};
} // namespace CesiumGltfConverters
