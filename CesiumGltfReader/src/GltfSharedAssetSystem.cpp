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
      std::make_unique<ImageAssetFactory>(options.ktx2TranscodeTargets));

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
