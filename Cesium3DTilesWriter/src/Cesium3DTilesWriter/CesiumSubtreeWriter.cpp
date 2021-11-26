#include "Cesium3DTilesWriter/CesiumSubtreeWriter.h"

#include "Cesium3DTilesWriter/TilesetWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

CesiumSubtreeWriter::CesiumSubtreeWriter() {
  SubtreeJsonWriter::populateExtensions(this->_context);
}

CesiumJsonWriter::ExtensionWriterContext& CesiumSubtreeWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
CesiumSubtreeWriter::getExtensions() const {
  return this->_context;
}

SubtreeWriterResult CesiumSubtreeWriter::writeSubtree(
    const Cesium3DTiles::Subtree& subtree,
    const WriteSubtreeOptions& options) const {
  CESIUM_TRACE("CesiumSubtreeWriter::writeSubtree");

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
