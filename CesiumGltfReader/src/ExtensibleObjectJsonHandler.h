#pragma once

#include "DictionaryJsonHandler.h"
#include "JsonObjectJsonHandler.h"
#include "ObjectJsonHandler.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
public:
  ExtensibleObjectJsonHandler(const ReadModelOptions& options) noexcept;
protected:
  void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
  IJsonHandler* ExtensibleObjectKey(const char* str, ExtensibleObject& o);

private:
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extras;
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extensions;
};
} // namespace CesiumGltf
