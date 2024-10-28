#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> createDefault() {
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> p =
      new TilesetSharedAssetSystem();

  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pGltf =
      GltfSharedAssetSystem::getDefault();

  p->pImage = pGltf->pImage;

  return p;
}

} // namespace

namespace Cesium3DTilesSelection {

/*static*/ CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>
TilesetSharedAssetSystem::getDefault() {
  static CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> pDefault =
      createDefault();
  return pDefault;
}

} // namespace Cesium3DTilesSelection
