#pragma once

#include <Cesium3DTilesSelection/Exp_GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>
#include <cstddef>

namespace Cesium3DTilesSelection {
struct BinaryToGltfConverter {
public:
  static GltfConverterResult convert(const gsl::span<const std::byte>& gltfBinary);

private:
  static CesiumGltfReader::GltfReader _gltfReader;
};
}
