#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGltfReader/NetworkImageAssetDescriptor.h>
#include <CesiumGltfReader/NetworkSchemaAssetDescriptor.h>

namespace CesiumGltf {
struct Schema;
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

  virtual ~GltfSharedAssetSystem() = default;

  using ImageDepot = CesiumAsync::
      SharedAssetDepot<CesiumGltf::ImageAsset, NetworkImageAssetDescriptor>;

  /**
   * @brief The asset depot for images.
   */
  CesiumUtility::IntrusivePointer<ImageDepot> pImage;

  using SchemaDepot = CesiumAsync::
      SharedAssetDepot<CesiumGltf::Schema, NetworkSchemaAssetDescriptor>;

  /**
   * @brief The asset depot for schemas.
   */
  CesiumUtility::IntrusivePointer<SchemaDepot> pExternalMetadataSchema;
};

} // namespace CesiumGltfReader
