// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "ClassPropertyJsonHandler.h"

#include <Cesium3DTiles/Class.h>
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumJsonReader {
class JsonReaderOptions;
} // namespace CesiumJsonReader

namespace Cesium3DTilesReader {
class ClassJsonHandler : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = Cesium3DTiles::Class;

  explicit ClassJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& options) noexcept;
  void reset(IJsonHandler* pParentHandler, Cesium3DTiles::Class* pObject);

  IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyClass(
      const std::string& objectType,
      const std::string_view& str,
      Cesium3DTiles::Class& o);

private:
  Cesium3DTiles::Class* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _description;
  CesiumJsonReader::DictionaryJsonHandler<
      Cesium3DTiles::ClassProperty,
      ClassPropertyJsonHandler>
      _properties;
  CesiumJsonReader::StringJsonHandler _parent;
};
} // namespace Cesium3DTilesReader
