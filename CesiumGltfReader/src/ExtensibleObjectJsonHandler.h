#pragma once

#include "CesiumGltf/Reader.h"
#include "CesiumJsonReader/DictionaryJsonHandler.h"
#include "CesiumJsonReader/JsonObjectJsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
#include "ExtensionsJsonHandler.h"

namespace CesiumGltf {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
public:
  ExtensibleObjectJsonHandler(const ReaderContext& context) noexcept;

protected:
  void reset(IJsonReader* pParent, ExtensibleObject* pObject);
  IJsonReader* readObjectKeyExtensibleObject(
      const std::string& objectType,
      const std::string_view& str,
      ExtensibleObject& o);

private:
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumGltf
