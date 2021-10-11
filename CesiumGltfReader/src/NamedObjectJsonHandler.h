#pragma once

#include "ExtensibleObjectJsonHandler.h"

#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler : public ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentReader, NamedObject* pObject);
  IJsonHandler* readObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      NamedObject& o);

private:
  CesiumJsonReader::StringJsonHandler _name;
};
} // namespace CesiumGltf
