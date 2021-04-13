#pragma once

#include "CesiumJsonReader/StringJsonHandler.h"
#include "ExtensibleObjectJsonHandler.h"

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler : public ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonReader* pParentReader, NamedObject* pObject);
  IJsonReader* readObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      NamedObject& o);

private:
  StringJsonHandler _name;
};
} // namespace CesiumGltf
