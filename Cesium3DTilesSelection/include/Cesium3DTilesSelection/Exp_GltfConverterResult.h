#pragma once

#include <Cesium3DTilesSelection/Exp_ErrorList.h>
#include <CesiumGltf/Model.h>
#include <optional>

namespace Cesium3DTilesSelection {
struct GltfConverterResult {
  std::optional<CesiumGltf::Model> model;

  ErrorList errors;
};
}
