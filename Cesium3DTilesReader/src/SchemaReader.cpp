#include "Cesium3DTilesReader/SchemaReader.h"

#include "SchemaJsonHandler.h"
#include "registerExtensions.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

SchemaReaderResult readSchemaJson(
    const CesiumJsonReader::JsonReaderOptions& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::SchemaReader::readSchemaJson");

  SchemaJsonHandler schemaHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Schema> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, schemaHandler);

  return SchemaReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

SchemaReader::SchemaReader() { registerExtensions(this->_context); }

CesiumJsonReader::JsonReaderOptions& SchemaReader::getOptions() {
  return this->_context;
}

const CesiumJsonReader::JsonReaderOptions& SchemaReader::getOptions() const {
  return this->_context;
}

SchemaReaderResult
SchemaReader::readSchema(const gsl::span<const std::byte>& data) const {
  const CesiumJsonReader::JsonReaderOptions& context = this->getOptions();
  SchemaReaderResult result = readSchemaJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader
