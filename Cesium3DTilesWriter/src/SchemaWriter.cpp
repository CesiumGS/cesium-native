#include "Cesium3DTilesWriter/SchemaWriter.h"

#include "TilesetJsonWriter.h"
#include "registerExtensions.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

SchemaWriter::SchemaWriter() { registerExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& SchemaWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
SchemaWriter::getExtensions() const {
  return this->_context;
}

SchemaWriterResult SchemaWriter::writeSchema(
    const Cesium3DTiles::Schema& schema,
    const SchemaWriterOptions& options) const {
  CESIUM_TRACE("SchemaWriter::writeSchema");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  SchemaWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  SchemaJsonWriter::write(schema, *writer, context);
  result.schemaBytes = writer->toBytes();

  return result;
}
} // namespace Cesium3DTilesWriter
