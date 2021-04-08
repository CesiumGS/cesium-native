#pragma once

#include "DictionaryJsonHandler.h"
#include "ExtensionsJsonHandler.h"
#include "JsonObjectJsonHandler.h"
#include "ObjectJsonHandler.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
public:
  ExtensibleObjectJsonHandler(const JsonReaderContext& context) noexcept;

protected:
  void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
  IJsonHandler* readObjectKeyExtensibleObject(
      const std::string& objectType,
      const std::string_view& str,
      ExtensibleObject& o);

private:
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumGltf
