#include "Cesium3DTilesReader/SubtreeReader.h"

#include "SubtreeJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

SubtreeReaderResult readSubtreeJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::SubtreeReader::readSubtreeJson");

  SubtreeJsonHandler subtreeHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, subtreeHandler);

  return SubtreeReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

SubtreeReader::SubtreeReader() { registerExtensions(this->_context); }

CesiumJsonReader::ExtensionReaderContext& SubtreeReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
SubtreeReader::getExtensions() const {
  return this->_context;
}

SubtreeReaderResult
SubtreeReader::readSubtree(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  SubtreeReaderResult result = readSubtreeJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
