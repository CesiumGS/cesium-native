#pragma once

#include "CesiumJsonReader/DictionaryJsonHandler.h"
#include "CesiumJsonReader/ExtensionReaderContext.h"
#include "CesiumJsonReader/ExtensionsJsonHandler.h"
#include "CesiumJsonReader/JsonObjectJsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
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
