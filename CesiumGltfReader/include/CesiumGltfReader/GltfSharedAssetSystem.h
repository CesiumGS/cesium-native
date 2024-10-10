#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumAsync/SharedFuture.h>

namespace CesiumGltf {
struct ImageAsset;
}

namespace CesiumGltfReader {

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class GltfSharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<GltfSharedAssetSystem> {
public:
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> getDefault();

  CesiumUtility::IntrusivePointer<
      CesiumAsync::SharedAssetDepot<CesiumGltf::ImageAsset>>
      pImage;
};

} // namespace CesiumGltfReader
