#pragma once

#include <CesiumJsonWriter/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonWriter/StringJsonHandler.h>

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler
    : public CesiumJsonWriter::ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(
      const CesiumJsonWriter::ExtensionWriterContext& context) noexcept;
  void reset(IJsonHandler* pParentWriter, NamedObject* pObject);
  IJsonHandler* writeObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      NamedObject& o);

private:
  CesiumJsonWriter::StringJsonHandler _name;
};
} // namespace CesiumGltf
