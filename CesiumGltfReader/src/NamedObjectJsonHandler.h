#pragma once

#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentReader, NamedObject* pObject);
  IJsonHandler* readObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      NamedObject& o);

private:
  CesiumJsonReader::StringJsonHandler _name;
};
} // namespace CesiumGltf
