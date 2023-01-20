#include "B3dmToGltfConverter.h"
#include "BinaryToGltfConverter.h"
#include "CmptToGltfConverter.h"
#include "PntsToGltfConverter.h"

#include <Cesium3DTilesSelection/GltfConverters.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>

namespace Cesium3DTilesSelection {

void registerAllTileContentTypes() {
  GltfConverters::registerMagic("glTF", BinaryToGltfConverter::convert);
  GltfConverters::registerMagic("b3dm", B3dmToGltfConverter::convert);
  GltfConverters::registerMagic("cmpt", CmptToGltfConverter::convert);
  GltfConverters::registerMagic("pnts", PntsToGltfConverter::convert);

  GltfConverters::registerFileExtension(
      ".gltf",
      BinaryToGltfConverter::convert);
  GltfConverters::registerFileExtension(".glb", BinaryToGltfConverter::convert);
}

} // namespace Cesium3DTilesSelection
