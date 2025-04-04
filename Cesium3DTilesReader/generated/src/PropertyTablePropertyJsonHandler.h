// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <Cesium3DTiles/PropertyTableProperty.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumJsonReader {
class JsonReaderOptions;
} // namespace CesiumJsonReader

namespace Cesium3DTilesReader {
class PropertyTablePropertyJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = Cesium3DTiles::PropertyTableProperty;

  explicit PropertyTablePropertyJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& options) noexcept;
  void reset(
      IJsonHandler* pParentHandler,
      Cesium3DTiles::PropertyTableProperty* pObject);

  IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyPropertyTableProperty(
      const std::string& objectType,
      const std::string_view& str,
      Cesium3DTiles::PropertyTableProperty& o);

private:
  Cesium3DTiles::PropertyTableProperty* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _values;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _arrayOffsets;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _stringOffsets;
  CesiumJsonReader::StringJsonHandler _arrayOffsetType;
  CesiumJsonReader::StringJsonHandler _stringOffsetType;
  CesiumJsonReader::JsonObjectJsonHandler _offset;
  CesiumJsonReader::JsonObjectJsonHandler _scale;
  CesiumJsonReader::JsonObjectJsonHandler _max;
  CesiumJsonReader::JsonObjectJsonHandler _min;
};
} // namespace Cesium3DTilesReader
