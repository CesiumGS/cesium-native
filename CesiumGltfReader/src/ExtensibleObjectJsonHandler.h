#pragma once

#include "DictionaryJsonHandler.h"
#include "JsonObjectJsonHandler.h"
#include "ObjectJsonHandler.h"

namespace CesiumGltf {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
protected:
  void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
  IJsonHandler* ExtensibleObjectKey(const char* str, ExtensibleObject& o);

private:
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extras;
};
} // namespace CesiumGltf
