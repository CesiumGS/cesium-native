#pragma once

#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

// Forward declaration
namespace CesiumGltf {
struct NamedObject;
}

namespace CesiumGltfReader {

class NamedObjectJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentReader, CesiumGltf::NamedObject* pObject);
  IJsonHandler* readObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::NamedObject& o);

private:
  CesiumJsonReader::StringJsonHandler _name;
};
} // namespace CesiumGltfReader
