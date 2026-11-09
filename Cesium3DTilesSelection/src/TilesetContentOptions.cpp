#include <Cesium3DTilesSelection/TilesetContentOptions.h>
#include <CesiumGltfReader/GltfReader.h>

namespace Cesium3DTilesSelection {

CesiumGltfReader::GltfReaderOptions
TilesetContentOptions::toGltfReaderOptions() const {
  CesiumGltfReader::GltfReaderOptions options;
  options.ktx2TranscodeTargets = this->ktx2TranscodeTargets;
  options.applyTextureTransform = this->applyTextureTransform;
  options.primitiveModeOptions = this->primitiveModeOptions;
  options.spzDecoderOptions = this->spzDecoderOptions;
  return options;
}

} // namespace Cesium3DTilesSelection
