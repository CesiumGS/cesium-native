#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> createDefault() {
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> p =
      new TilesetSharedAssetSystem();

  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pGltf =
      GltfSharedAssetSystem::getDefault();

  p->pImage = pGltf->pImage;
  p->pExternalMetadataSchema = pGltf->pExternalMetadataSchema;

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
