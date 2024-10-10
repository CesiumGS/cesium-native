#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumAsync/SharedFuture.h>

namespace CesiumGltf {
struct ImageCesium;
}

namespace CesiumGltfReader {

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class GltfSharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<GltfSharedAssetSystem> {
public:
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> default();

  CesiumUtility::IntrusivePointer<
      CesiumAsync::SharedAssetDepot<CesiumGltf::ImageCesium>>
      pImage;
};

} // namespace CesiumGltfReader
