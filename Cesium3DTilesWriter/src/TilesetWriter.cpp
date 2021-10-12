#include "Cesium3DTiles/TilesetWriter.h"

#include "CesiumUtility/Tracing.h"
#include "TilesetJsonHandler.h"

#include <CesiumJsonWriter/JsonHandler.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <rapidjson/reader.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

using namespace Cesium3DTiles;
using namespace CesiumJsonWriter;
using namespace CesiumUtility;

namespace {

TilesetWriterResult writeTilesetJson(
    const CesiumJsonWriter::ExtensionWriterContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTiles::TilesetWriter::writeTilesetJson");

  TilesetJsonHandler tilesetHandler(context);
  WriteJsonResult<Tileset> jsonResult =
      JsonWriter::writeJson(data, tilesetHandler);

  return TilesetWriterResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

TilesetWriter::TilesetWriter() {}

CesiumJsonWriter::ExtensionWriterContext& TilesetWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
TilesetWriter::getExtensions() const {
  return this->_context;
}

TilesetWriterResult TilesetWriter::writeTileset(
    const gsl::span<const std::byte>& data,
    const WriteTilesetOptions& /*options*/) const {
  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();
  TilesetWriterResult result = writeTilesetJson(context, data);

  return result;
}
