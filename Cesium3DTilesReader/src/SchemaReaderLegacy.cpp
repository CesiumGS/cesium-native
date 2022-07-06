#include "Cesium3DTilesReader/SchemaReaderLegacy.h"

#include "Extension3dTilesMetadataSchemaLegacyJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

SchemaReaderResultLegacy readSchemaJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::SchemaReaderLegacy::readSchemaJson");

  Extension3dTilesMetadataSchemaLegacyJsonHandler schemaHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Extension3dTilesMetadataSchemaLegacy> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, schemaHandler);

  return SchemaReaderResultLegacy{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

SchemaReaderLegacy::SchemaReaderLegacy() { registerExtensions(this->_context); }

CesiumJsonReader::ExtensionReaderContext& SchemaReaderLegacy::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
SchemaReaderLegacy::getExtensions() const {
  return this->_context;
}

SchemaReaderResultLegacy
SchemaReaderLegacy::readSchema(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  SchemaReaderResultLegacy result = readSchemaJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
