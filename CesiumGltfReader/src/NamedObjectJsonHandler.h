#pragma once

#include "ExtensibleObjectJsonHandler.h"
#include "StringJsonHandler.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler : public ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonReader* pParent, NamedObject* pObject);
  IJsonReader* readObjectKeyNamedObject(
      const std::string& objectType,
      const std::string_view& str,
      NamedObject& o);

private:
  StringJsonHandler _name;
};
} // namespace CesiumGltf
