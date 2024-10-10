#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>

namespace CesiumGltfReader {

namespace {

CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> createDefault() {
  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> p =
      new GltfSharedAssetSystem();

  p->pImage.emplace();

  return p;
}

} // namespace

/*static*/ CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
    GltfSharedAssetSystem::default() {
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pDefault =
      createDefault();
  return pDefault;
}

} // namespace CesiumGltfReader
