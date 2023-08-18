#pragma once

#include <Cesium3DTiles/GroupMetadata.h>
#include <Cesium3DTiles/MetadataEntity.h>
#include <Cesium3DTiles/Schema.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/JsonReaderOptions.h>

namespace Cesium3DTilesReader {

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Schema> readSchema(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options = {});

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::MetadataEntity>
readMetadataEntity(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options = {});

CesiumJsonReader::ReadJsonResult<Cesium3DTiles::GroupMetadata>
readGroupMetadata(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options = {});

CesiumJsonReader::ReadJsonResult<std::vector<Cesium3DTiles::GroupMetadata>>
readGroupMetadataArray(
    const rapidjson::Value& value,
    const CesiumJsonReader::JsonReaderOptions& options = {});

} // namespace Cesium3DTilesReader
