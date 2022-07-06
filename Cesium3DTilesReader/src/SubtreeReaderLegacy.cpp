#include "Cesium3DTilesReader/SubtreeLegacyReader.h"

#include "Extension3dTilesImplicitTilingSubtreeLegacyJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

SubtreeLegacyReaderResult readSubtreeJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::SubtreeLegacyReaderResult::readSubtreeJson");

  Extension3dTilesImplicitTilingSubtreeLegacyJsonHandler subtreeHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Extension3dTilesImplicitTilingSubtreeLegacy> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, subtreeHandler);

  return SubtreeLegacyReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

SubtreeLegacyReader::SubtreeLegacyReader() { registerExtensions(this->_context); }

CesiumJsonReader::ExtensionReaderContext& SubtreeLegacyReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
SubtreeLegacyReader::getExtensions() const {
  return this->_context;
}

SubtreeLegacyReaderResult
SubtreeLegacyReader::readSubtree(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  SubtreeLegacyReaderResult result = readSubtreeJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
