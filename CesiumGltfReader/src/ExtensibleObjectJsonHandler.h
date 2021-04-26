#pragma once

#include "CesiumGltf/ReaderContext.h"
#include "CesiumJsonReader/DictionaryJsonHandler.h"
#include "CesiumJsonReader/JsonObjectJsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
#include "CesiumUtility/JsonValue.h"
#include "ExtensionsJsonHandler.h"

namespace CesiumGltf {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  ExtensibleObjectJsonHandler(const ReaderContext& context) noexcept;

protected:
  void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
  IJsonHandler* readObjectKeyExtensibleObject(
      const std::string& objectType,
      const std::string_view& str,
      ExtensibleObject& o);

private:
  CesiumJsonReader::DictionaryJsonHandler<
      CesiumUtility::JsonValue,
      CesiumJsonReader::JsonObjectJsonHandler>
      _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumGltf
