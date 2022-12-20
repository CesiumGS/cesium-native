#include "Cesium3DTilesReader/VoxelContentReader.h"

#include "VoxelContentJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

VoxelContentReaderResult readVoxelJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::VoxelContentReader::readVoxelJson");

  VoxelContentJsonHandler voxelHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::VoxelContent> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, voxelHandler);

  return VoxelContentReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

VoxelContentReader::VoxelContentReader() { registerExtensions(this->_context); }

CesiumJsonReader::ExtensionReaderContext& VoxelContentReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
VoxelContentReader::getExtensions() const {
  return this->_context;
}

VoxelContentReaderResult VoxelContentReader::readVoxelContent(
    const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  VoxelContentReaderResult result = readVoxelJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
