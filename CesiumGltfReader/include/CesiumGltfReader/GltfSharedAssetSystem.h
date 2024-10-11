#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>

namespace CesiumGltf {
struct ImageAsset;
}

namespace CesiumGltfReader {

class AssetSystemOptions {
public:
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets;
};

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class GltfSharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<GltfSharedAssetSystem> {
public:
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
  getDefault(const AssetSystemOptions& options);

  CesiumUtility::IntrusivePointer<
      CesiumAsync::SharedAssetDepot<CesiumGltf::ImageAsset>>
      pImage;
};

} // namespace CesiumGltfReader
