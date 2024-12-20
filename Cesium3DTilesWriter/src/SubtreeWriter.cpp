#include "TilesetJsonWriter.h"
#include "registerWriterExtensions.h"

#include <Cesium3DTilesWriter/SubtreeWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace Cesium3DTilesWriter {

SubtreeWriter::SubtreeWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& SubtreeWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
SubtreeWriter::getExtensions() const {
  return this->_context;
}

SubtreeWriterResult SubtreeWriter::writeSubtree(
    const Cesium3DTiles::Subtree& subtree,
    const SubtreeWriterOptions& options) const {
  CESIUM_TRACE("SubtreeWriter::writeSubtree");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  SubtreeWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  SubtreeJsonWriter::write(subtree, *writer, context);
  result.subtreeBytes = writer->toBytes();
  result.errors = writer->getErrors();
  result.warnings = writer->getWarnings();

  return result;
}
} // namespace Cesium3DTilesWriter
