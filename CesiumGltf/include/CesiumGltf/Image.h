#pragma once

#include "CesiumGltf/ImageCesium.h"
#include "CesiumGltf/ImageSpec.h"
#include "CesiumGltf/Library.h"

#include <CesiumGltf/SharedAssetDepot.h>

namespace CesiumGltf {
/** @copydoc ImageSpec */
struct CESIUMGLTF_API Image final : public ImageSpec {
  /**
   * @brief Holds properties that are specific to the glTF loader rather than
   * part of the glTF spec. When an image is loaded from a URL, multiple `Image`
   * instances may all point to the same `ImageCesium` instance.
   */
  CesiumUtility::IntrusivePointer<ImageCesium> pCesium;
};
} // namespace CesiumGltf
