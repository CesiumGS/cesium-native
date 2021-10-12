#pragma once

#include "DictionaryJsonHandler.h"
#include "ExtensionWriterContext.h"
#include "ExtensionsJsonHandler.h"
#include "JsonObjectJsonHandler.h"
#include "ObjectJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonWriter {

class ExtensibleObjectJsonHandler : public CesiumJsonWriter::ObjectJsonHandler {
public:
  explicit ExtensibleObjectJsonHandler(
      const ExtensionWriterContext& context) noexcept;

protected:
  void reset(IJsonHandler* pParent, CesiumUtility::ExtensibleObject* pObject);
  IJsonHandler* writeObjectKeyExtensibleObject(
      const std::string& objectType,
      const std::string_view& str,
      CesiumUtility::ExtensibleObject& o);

private:
  CesiumJsonWriter::DictionaryJsonHandler<
      CesiumUtility::JsonValue,
      CesiumJsonWriter::JsonObjectJsonHandler>
      _extras;
  ExtensionsJsonHandler _extensions;
};
} // namespace CesiumJsonWriter
