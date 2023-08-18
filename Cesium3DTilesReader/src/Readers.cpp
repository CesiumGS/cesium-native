#include "GroupMetadataJsonHandler.h"
#include "MetadataEntityJsonHandler.h"
#include "SchemaJsonHandler.h"

#include <Cesium3DTilesReader/Readers.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>

namespace Cesium3DTilesReader {

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Schema> readSchema(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options) {
  SchemaJsonHandler handler(options);
  return CesiumJsonReader::JsonReader::readJson(value, handler);
}

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::MetadataEntity>
readMetadataEntity(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options) {
  MetadataEntityJsonHandler handler(options);
  return CesiumJsonReader::JsonReader::readJson(value, handler);
}

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::GroupMetadata>
readGroupMetadata(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options) {
  GroupMetadataJsonHandler handler(options);
  return CesiumJsonReader::JsonReader::readJson(value, handler);
}

CesiumJsonReader::ReadJsonResult<std::vector<Cesium3DTiles::GroupMetadata>>
readGroupMetadataArray(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options) {
  CesiumJsonReader::
      ArrayJsonHandler<Cesium3DTiles::GroupMetadata, GroupMetadataJsonHandler>
          handler(options);
  return CesiumJsonReader::JsonReader::readJson(value, handler);
}

} // namespace Cesium3DTilesReader
