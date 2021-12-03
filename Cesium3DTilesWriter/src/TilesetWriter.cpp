#include "Cesium3DTilesWriter/TilesetWriter.h"

#include "TilesetJsonWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

TilesetWriter::TilesetWriter() { registerExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& TilesetWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
TilesetWriter::getExtensions() const {
  return this->_context;
}

TilesetWriterResult
TilesetWriter::writeTileset(const Cesium3DTiles::Tileset& tileset) const {
  CESIUM_TRACE("TilesetWriter::writeTileset");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  TilesetWriterResult result;
  CesiumJsonWriter::PrettyJsonWriter writer;

  TilesetJsonWriter::write(tileset, writer, context);
  result.tilesetBytes = writer.toBytes();

  return result;
}
} // namespace Cesium3DTilesWriter
