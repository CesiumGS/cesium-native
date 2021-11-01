#include "Cesium3DTiles/TilesetReader.h"

#include "CesiumUtility/Tracing.h"
#include "Extension3dTilesContentGltfJsonHandler.h"
#include "TilesetJsonHandler.h"

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>

#include <rapidjson/reader.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

using namespace Cesium3DTiles;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {

TilesetReaderResult readTilesetJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTiles::TilesetReader::readTilesetJson");

  TilesetJsonHandler tilesetHandler(context);
  ReadJsonResult<Tileset> jsonResult =
      JsonReader::readJson(data, tilesetHandler);

  return TilesetReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

TilesetReader::TilesetReader() {
  this->_context
      .registerExtension<Tileset, Extension3dTilesContentGltfJsonHandler>();
}

CesiumJsonReader::ExtensionReaderContext& TilesetReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
TilesetReader::getExtensions() const {
  return this->_context;
}

TilesetReaderResult TilesetReader::readTileset(
    const gsl::span<const std::byte>& data,
    const ReadTilesetOptions& /*options*/) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  TilesetReaderResult result = readTilesetJson(context, data);

  return result;
}
