#include "Cesium3DTilesReader/SubtreeReaderLegacy.h"

#include "Extension3dTilesImplicitTilingSubtreeLegacyJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

SubtreeReaderResultLegacy readSubtreeJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE(
      "Cesium3DTilesReader::SubtreeReaderResultLegacy::readSubtreeJson");

  Extension3dTilesImplicitTilingSubtreeLegacyJsonHandler subtreeHandler(
      context);
  CesiumJsonReader::ReadJsonResult<
      Cesium3DTiles::Extension3dTilesImplicitTilingSubtreeLegacy>
      jsonResult = CesiumJsonReader::JsonReader::readJson(data, subtreeHandler);

  return SubtreeReaderResultLegacy{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

SubtreeReaderLegacy::SubtreeReaderLegacy() {
  registerExtensions(this->_context);
}

CesiumJsonReader::ExtensionReaderContext& SubtreeReaderLegacy::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
SubtreeReaderLegacy::getExtensions() const {
  return this->_context;
}

SubtreeReaderResultLegacy
SubtreeReaderLegacy::readSubtree(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  SubtreeReaderResultLegacy result = readSubtreeJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
