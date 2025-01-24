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
  /**
   * @brief Obtains an `IntrusivePointer` to the `GltfSharedAssetSystem`
   * singleton.
   */
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> getDefault();

  virtual ~GltfSharedAssetSystem() = default;

  /**
   * @brief A depot containing images loaded from glTFs.
   *
   * See \ref CesiumGltf::ImageAsset "ImageAsset" and \ref
   * NetworkImageAssetDescriptor.
   */
  using ImageDepot = CesiumAsync::
      SharedAssetDepot<CesiumGltf::ImageAsset, NetworkImageAssetDescriptor>;

  /**
   * @brief The asset depot for images.
   */
  CesiumUtility::IntrusivePointer<ImageDepot> pImage;

  /**
   * @brief A depot containing schemas loaded from URIs contained in the glTF
   * EXT_structural_metadata extension.
   *
   * See \ref CesiumGltf::Schema "Schema" and \ref NetworkSchemaAssetDescriptor.
   */
  using SchemaDepot = CesiumAsync::
      SharedAssetDepot<CesiumGltf::Schema, NetworkSchemaAssetDescriptor>;

  /**
   * @brief The asset depot for schemas.
   */
  CesiumUtility::IntrusivePointer<SchemaDepot> pExternalMetadataSchema;
};

} // namespace CesiumGltfReader
