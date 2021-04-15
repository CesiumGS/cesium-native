#pragma once
#include "WriterLibrary.h"
namespace CesiumGltf {
CESIUMGLTFWRITER_API enum WriteFlags {
  GLB = 1,
  GLTF = 2,
  PrettyPrint = 4,
  AutoConvertDataToBase64 = 8
};

CESIUMGLTFWRITER_API inline constexpr WriteFlags
operator|(WriteFlags x, WriteFlags y) {
  return static_cast<WriteFlags>(static_cast<int>(x) | static_cast<int>(y));
}
} // namespace CesiumGltf
