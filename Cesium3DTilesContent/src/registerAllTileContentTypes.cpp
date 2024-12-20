#include <Cesium3DTilesContent/B3dmToGltfConverter.h>
#include <Cesium3DTilesContent/BinaryToGltfConverter.h>
#include <Cesium3DTilesContent/CmptToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/I3dmToGltfConverter.h>
#include <Cesium3DTilesContent/PntsToGltfConverter.h>
#include <Cesium3DTilesContent/registerAllTileContentTypes.h>

namespace Cesium3DTilesContent {

void registerAllTileContentTypes() {
  GltfConverters::registerMagic("glTF", BinaryToGltfConverter::convert);
  GltfConverters::registerMagic("b3dm", B3dmToGltfConverter::convert);
  GltfConverters::registerMagic("cmpt", CmptToGltfConverter::convert);
  GltfConverters::registerMagic("i3dm", I3dmToGltfConverter::convert);
  GltfConverters::registerMagic("pnts", PntsToGltfConverter::convert);

  GltfConverters::registerFileExtension(
      ".gltf",
      BinaryToGltfConverter::convert);
  GltfConverters::registerFileExtension(".glb", BinaryToGltfConverter::convert);
}

} // namespace Cesium3DTilesContent
