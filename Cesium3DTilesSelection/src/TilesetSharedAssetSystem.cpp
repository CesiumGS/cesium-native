#include <Cesium3DTilesSelection/MaxarContentGeoJsonSchemaAssetDescriptor.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <functional>

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
  p->pExternalMetadataSchema = pGltf->pExternalMetadataSchema;

  p->pMaxarContentGeoJsonSchema.emplace(std::function(
      [](const SharedAssetContext& context,
         const MaxarContentGeoJsonSchemaAssetDescriptor& key)
          -> Future<ResultPointer<Schema>> {
        return key.load(context.asyncSystem, context.pAssetAccessor);
      }));

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
