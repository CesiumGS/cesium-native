#include <CesiumGltfConverters/B3dmToGltfConverter.h>
#include <CesiumGltfConverters/BinaryToGltfConverter.h>
#include <CesiumGltfConverters/CmptToGltfConverter.h>
#include <CesiumGltfConverters/GltfConverters.h>
#include <CesiumGltfConverters/I3dmToGltfConverter.h>
#include <CesiumGltfConverters/PntsToGltfConverter.h>
#include <CesiumGltfConverters/registerAllConverterTypes.h>

namespace CesiumGltfConverters {

void registerAllConverterTypes() {
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

} // namespace CesiumGltfConverters
