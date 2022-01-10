#include "Cesium3DTilesReader/TilesetReader.h"

#include "TilesetJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

TilesetReaderResult readTilesetJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::TilesetReader::readTilesetJson");

  TilesetJsonHandler tilesetHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Tileset> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, tilesetHandler);

  return TilesetReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

TilesetReader::TilesetReader() { registerExtensions(this->_context); }

CesiumJsonReader::ExtensionReaderContext& TilesetReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
TilesetReader::getExtensions() const {
  return this->_context;
}

TilesetReaderResult
TilesetReader::readTileset(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  TilesetReaderResult result = readTilesetJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
