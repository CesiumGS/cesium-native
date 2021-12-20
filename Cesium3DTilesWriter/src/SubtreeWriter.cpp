#include "Cesium3DTilesWriter/SubtreeWriter.h"

#include "TilesetJsonWriter.h"
#include "registerExtensions.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

SubtreeWriter::SubtreeWriter() { registerExtensions(this->_context); }

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

  return result;
}
} // namespace Cesium3DTilesWriter
