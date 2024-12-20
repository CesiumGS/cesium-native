#pragma once

#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/ImageSpec.h>
#include <CesiumGltf/Library.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace CesiumGltf {
/** @copydoc ImageSpec */
struct CESIUMGLTF_API Image final : public ImageSpec {
  Image() = default;

  /**
   * @brief The loaded image asset. When an image is loaded from a URL, multiple
   * `Image` instances may all point to the same `ImageAsset` instance.
   */
  CesiumUtility::IntrusivePointer<ImageAsset> pAsset;
};
} // namespace CesiumGltf
