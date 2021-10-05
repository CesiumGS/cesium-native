#pragma once

#include "ImageCesium.h"
#include "ImageSpec.h"
#include "Library.h"

namespace CesiumGltf {
/** @copydoc ImageSpec */
struct CESIUMGLTF_API Image final : public ImageSpec {
  /**
   * @brief Holds properties that are specific to the glTF loader rather than
   * part of the glTF spec.
   */
  ImageCesium cesium;
};
} // namespace CesiumGltf
