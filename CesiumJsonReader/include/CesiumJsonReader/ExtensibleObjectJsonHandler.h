#pragma once

#include "DictionaryJsonHandler.h"
#include "ExtensionReaderContext.h"
#include "ExtensionsJsonHandler.h"
#include "JsonObjectJsonHandler.h"
#include "ObjectJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonReader {

class ExtensibleObjectJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  explicit ExtensibleObjectJsonHandler(
      const ExtensionReaderContext& context) noexcept;

protected:
  void reset(IJsonHandler* pParent, CesiumUtility::ExtensibleObject* pObject);
  IJsonHandler* readObjectKeyExtensibleObject(
      const std::string& objectType,
      const std::string_view& str,
      CesiumUtility::ExtensibleObject& o);

private:
  CesiumJsonReader::DictionaryJsonHandler<
      CesiumUtility::JsonValue,
      CesiumJsonReader::JsonObjectJsonHandler>
      _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumJsonReader
