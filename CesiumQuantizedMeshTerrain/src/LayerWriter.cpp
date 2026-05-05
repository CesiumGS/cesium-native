#include "LayerJsonWriter.h"
#include "registerWriterExtensions.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumQuantizedMeshTerrain/LayerWriter.h>
#include <CesiumUtility/Tracing.h>

#include <memory>

namespace CesiumQuantizedMeshTerrain {

LayerWriter::LayerWriter() { registerWriterExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& LayerWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
LayerWriter::getExtensions() const {
  return this->_context;
}

LayerWriterResult LayerWriter::write(
    const Layer& layer,
    const LayerWriterOptions& options) const {
  CESIUM_TRACE("LayerWriter::write");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  LayerWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> pWriter;

  if (options.prettyPrint) {
    pWriter = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    pWriter = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  LayerJsonWriter::write(layer, *pWriter, context);
  result.bytes = pWriter->toBytes();
  result.errors = pWriter->getErrors();
  result.warnings = pWriter->getWarnings();

  return result;
}

} // namespace CesiumQuantizedMeshTerrain
