#include "Cesium3DTilesReader/TilesetReader.h"

#include "TilesetJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

TilesetReaderResult readTilesetJson(
    const CesiumJsonReader::JsonReaderOptions& context,
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

CesiumJsonReader::JsonReaderOptions& TilesetReader::getOptions() {
  return this->_context;
}

const CesiumJsonReader::JsonReaderOptions& TilesetReader::getOptions() const {
  return this->_context;
}

TilesetReaderResult
TilesetReader::readTileset(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::JsonReaderOptions& context = this->getOptions();
  TilesetReaderResult result = readTilesetJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
