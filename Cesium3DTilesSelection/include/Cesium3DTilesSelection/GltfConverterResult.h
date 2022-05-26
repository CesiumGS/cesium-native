#pragma once

#include <Cesium3DTilesSelection/ErrorList.h>
#include <CesiumGltf/Model.h>

#include <optional>

namespace Cesium3DTilesSelection {
struct GltfConverterResult {
  std::optional<CesiumGltf::Model> model;

  ErrorList errors;
};
} // namespace Cesium3DTilesSelection
