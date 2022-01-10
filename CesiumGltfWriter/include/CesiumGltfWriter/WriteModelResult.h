#pragma once

#include "Library.h"

#include <cstddef>
#include <string>
#include <vector>

namespace CesiumGltfWriter {

struct CESIUMGLTFWRITER_API WriteModelResult {
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::byte> gltfAssetBytes;
};

} // namespace CesiumGltfWriter
