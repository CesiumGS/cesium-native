#include "ModelJsonWriter.h"
#include "registerWriterExtensions.h"

#include <CesiumGltfWriter/SchemaWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace CesiumGltfWriter {

SchemaWriter::SchemaWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& SchemaWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
SchemaWriter::getExtensions() const {
  return this->_context;
}

SchemaWriterResult SchemaWriter::writeSchema(
    const CesiumGltf::Schema& schema,
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
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}
} // namespace CesiumGltfWriter
