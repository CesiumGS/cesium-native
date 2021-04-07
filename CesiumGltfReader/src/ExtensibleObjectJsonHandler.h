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
  ExtensibleObjectJsonHandler(const ReadModelOptions& options) noexcept;

protected:
  void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
  IJsonHandler* ExtensibleObjectKey(
      const std::string& objectType,
      const std::string_view& str,
      ExtensibleObject& o);

private:
  DictionaryJsonHandler<JsonValue, JsonObjectJsonHandler> _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumGltf
