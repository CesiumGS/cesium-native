#pragma once
#include "WriterLibrary.h"

#include <cstddef>
#include <string>
#include <vector>

namespace CesiumGltf {

struct CESIUMGLTFWRITER_API WriteModelResult {
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::byte> gltfAssetBytes;
};

} // namespace CesiumGltf
