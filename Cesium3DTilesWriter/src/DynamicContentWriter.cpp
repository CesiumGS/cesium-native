#include "TilesetJsonWriter.h"
#include "registerWriterExtensions.h"

#include <Cesium3DTilesWriter/DynamicContentWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace Cesium3DTilesWriter {

DynamicContentWriter::DynamicContentWriter() {
  registerWriterExtensions(this->_context);
}

CesiumJsonWriter::ExtensionWriterContext&
DynamicContentWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
DynamicContentWriter::getExtensions() const {
  return this->_context;
}

DynamicContentWriterResult DynamicContentWriter::writeDynamicContent(
    const Cesium3DTiles::DynamicContent& dynamicContent,
    const DynamicContentWriterOptions& options) const {
  CESIUM_TRACE("DynamicContentWriter::writeDynamicContent");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  DynamicContentWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  DynamicContentJsonWriter::write(dynamicContent, *writer, context);
  result.dynamicContentBytes = writer->toBytes();
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}
} // namespace Cesium3DTilesWriter
