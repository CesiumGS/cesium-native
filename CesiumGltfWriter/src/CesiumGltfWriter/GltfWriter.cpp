#include "CesiumGltfWriter/GltfWriter.h"

#include "CesiumGltfWriter/ModelJsonWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace CesiumGltfWriter {

GltfWriter::GltfWriter() { populateExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& GltfWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
GltfWriter::getExtensions() const {
  return this->_context;
}

GltfWriterResult GltfWriter::writeGltf(
    const CesiumGltf::Model& model,
    const GltfWriterOptions& options) const {
  CESIUM_TRACE("GltfWriter::writeGltf");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  GltfWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  ModelJsonWriter::write(model, *writer, context);
  result.gltfBytes = writer->toBytes();

  return result;
}
} // namespace CesiumGltfWriter
