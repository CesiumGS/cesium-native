#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumGltfReader/ImageReader.h>

namespace CesiumGltfReader {

namespace {

CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
createDefault(const AssetSystemOptions& options) {
  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> p =
      new GltfSharedAssetSystem();

  p->pImage.emplace(
      std::function([pAccessor = options.pAssetAccessor](
                        const CesiumAsync::AsyncSystem& asyncSystem,
                        const NetworkImageAssetKey& key) {
        return key.loadBytesFromNetwork(asyncSystem, pAccessor);
      }));

  return p;
}

} // namespace

/*static*/ CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
GltfSharedAssetSystem::getDefault(const AssetSystemOptions& options) {
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pDefault =
      createDefault(options);
  return pDefault;
}

} // namespace CesiumGltfReader
