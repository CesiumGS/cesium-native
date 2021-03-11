#pragma once

#include "ExtensibleObjectJsonHandler.h"
#include "StringJsonHandler.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {
struct NamedObject;

class NamedObjectJsonHandler : public ExtensibleObjectJsonHandler {
protected:
  NamedObjectJsonHandler(ReadModelOptions options) noexcept;
  void reset(IJsonHandler* pParent, NamedObject* pObject);
  IJsonHandler* NamedObjectKey(const char* str, NamedObject& o);

private:
  StringJsonHandler _name;
};
} // namespace CesiumGltf
