#pragma once

#include "CesiumJsonExtensions/ExtensionContext.h"
#include "CesiumJsonExtensions/ExtensionsJsonHandler.h"
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonExtensions {
struct ExtensibleObject;

class ExtensibleObjectJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  explicit ExtensibleObjectJsonHandler(
      const ExtensionContext& context) noexcept;

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
} // namespace CesiumJsonExtensions
