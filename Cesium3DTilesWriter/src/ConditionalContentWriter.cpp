#include "TilesetJsonWriter.h"
#include "registerWriterExtensions.h"

#include <Cesium3DTilesWriter/ConditionalContentWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace Cesium3DTilesWriter {

ConditionalContentWriter::ConditionalContentWriter() {
  registerWriterExtensions(this->_context);
}

CesiumJsonWriter::ExtensionWriterContext&
ConditionalContentWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
ConditionalContentWriter::getExtensions() const {
  return this->_context;
}

ConditionalContentWriterResult
ConditionalContentWriter::writeConditionalContent(
    const Cesium3DTiles::ConditionalContent& conditionalContent,
    const ConditionalContentWriterOptions& options) const {
  CESIUM_TRACE("ConditionalContentWriter::writeConditionalContent");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  ConditionalContentWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  ConditionalContentJsonWriter::write(conditionalContent, *writer, context);
  result.conditionalContentBytes = writer->toBytes();
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}
} // namespace Cesium3DTilesWriter
