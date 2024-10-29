#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGltfReader/NetworkImageAssetDescriptor.h>

namespace CesiumGltfReader {

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class GltfSharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<GltfSharedAssetSystem> {
public:
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> getDefault();

  virtual ~GltfSharedAssetSystem() = default;

  using ImageDepot = CesiumAsync::
      SharedAssetDepot<CesiumGltf::ImageAsset, NetworkImageAssetDescriptor>;

  /**
   * @brief The asset depot for images.
   */
  CesiumUtility::IntrusivePointer<ImageDepot> pImage;
};

} // namespace CesiumGltfReader
