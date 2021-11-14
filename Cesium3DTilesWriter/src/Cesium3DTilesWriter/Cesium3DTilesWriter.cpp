#include "Cesium3DTilesWriter/Cesium3DTilesWriter.h"

#include "Cesium3DTilesWriter/TilesetWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

Cesium3DTilesWriter::Cesium3DTilesWriter() {
  TilesetWriter::populateExtensions(this->_context);
}

CesiumJsonWriter::ExtensionWriterContext& Cesium3DTilesWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
Cesium3DTilesWriter::getExtensions() const {
  return this->_context;
}

TilesetWriterResult Cesium3DTilesWriter::writeTileset(
    const Cesium3DTiles::Tileset& tileset,
    const WriteTilesetOptions& options) const {
  CESIUM_TRACE("Cesium3DTilesWriter::writeTileset");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  TilesetWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  TilesetWriter::write(tileset, *writer, context);
  result.tilesetBytes = writer->toBytes();

  return result;
}
} // namespace Cesium3DTilesWriter
