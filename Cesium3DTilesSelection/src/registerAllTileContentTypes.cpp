#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"

#include <Cesium3DTilesSelection/Exp_B3dmToGltfConverter.h>
#include <Cesium3DTilesSelection/Exp_BinaryToGltfConverter.h>
#include <Cesium3DTilesSelection/Exp_CmptToGltfConverter.h>
#include <Cesium3DTilesSelection/Exp_GltfConverters.h>

namespace Cesium3DTilesSelection {

void registerAllTileContentTypes() {
  GltfConverters::registerMagic("glTF", BinaryToGltfConverter::convert);
  GltfConverters::registerMagic("b3dm", B3dmToGltfConverter::convert);
  GltfConverters::registerMagic("cmpt", CmptToGltfConverter::convert);

  GltfConverters::registerFileExtension(
      ".gltf",
      BinaryToGltfConverter::convert);
  GltfConverters::registerFileExtension(".glb", BinaryToGltfConverter::convert);
}

} // namespace Cesium3DTilesSelection
