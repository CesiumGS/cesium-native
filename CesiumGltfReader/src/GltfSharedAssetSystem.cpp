#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumGltfReader/NetworkImageAssetDescriptor.h>
#include <CesiumGltfReader/NetworkSchemaAssetDescriptor.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <functional>
#include <memory>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> createDefault() {
  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> p =
      new GltfSharedAssetSystem();

  p->pImage.emplace(std::function(
      [](const AsyncSystem& asyncSystem,
         const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
         const NetworkImageAssetDescriptor& key)
          -> Future<ResultPointer<ImageAsset>> {
        return key.load(asyncSystem, pAssetAccessor);
      }));

  p->pExternalMetadataSchema.emplace(std::function(
      [](const AsyncSystem& asyncSystem,
         const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
         const NetworkSchemaAssetDescriptor& key)
          -> Future<ResultPointer<Schema>> {
        return key.load(asyncSystem, pAssetAccessor);
      }));

  return p;
}

} // namespace

namespace CesiumGltfReader {

/*static*/ CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
GltfSharedAssetSystem::getDefault() {
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pDefault =
      createDefault();
  return pDefault;
}

} // namespace CesiumGltfReader
