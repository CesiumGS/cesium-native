#include "TilesetJsonWriter.h"
#include "registerWriterExtensions.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumTilesetJsonWriter/TilesetWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace CesiumTilesetJsonWriter {

TilesetWriter::TilesetWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& TilesetWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
TilesetWriter::getExtensions() const {
  return this->_context;
}

TilesetWriterResult TilesetWriter::writeTileset(
    const CesiumTilesetJson::Tileset& tileset,
    const TilesetWriterOptions& options) const {
  CESIUM_TRACE("TilesetWriter::writeTileset");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  TilesetWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> pWriter;

  if (options.prettyPrint) {
    pWriter = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    pWriter = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  TilesetJsonWriter::write(tileset, *pWriter, context);
  result.tilesetBytes = pWriter->toBytes();
  result.errors = pWriter->getErrors();
  result.warnings = pWriter->getWarnings();

  return result;
}
} // namespace CesiumTilesetJsonWriter
