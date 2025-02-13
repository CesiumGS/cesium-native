// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "PropertyAttributePropertyJsonHandler.h"

#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumJsonReader {
class JsonReaderOptions;
} // namespace CesiumJsonReader

namespace CesiumGltfReader {
class PropertyAttributeJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = CesiumGltf::PropertyAttribute;

  explicit PropertyAttributeJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& options) noexcept;
  void
  reset(IJsonHandler* pParentHandler, CesiumGltf::PropertyAttribute* pObject);

  IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyPropertyAttribute(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::PropertyAttribute& o);

private:
  CesiumGltf::PropertyAttribute* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _classProperty;
  CesiumJsonReader::DictionaryJsonHandler<
      CesiumGltf::PropertyAttributeProperty,
      PropertyAttributePropertyJsonHandler>
      _properties;
};
} // namespace CesiumGltfReader
