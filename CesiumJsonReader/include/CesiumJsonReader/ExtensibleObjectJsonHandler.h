#pragma once

#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/ExtensionsJsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonReader {

/**
 * @brief An \ref IJsonHandler for reading \ref CesiumUtility::ExtensibleObject
 * "ExtensibleObject" types.
 */
class ExtensibleObjectJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  /**
   * @brief Creates an \ref ExtensibleObjectJsonHandler with the specified
   * options.
   *
   * @param context Options to configure how the JSON is parsed.
   */
  explicit ExtensibleObjectJsonHandler(
      const JsonReaderOptions& context) noexcept;

protected:
  /**
   * @brief Resets the current parent of this handler, and the current object
   * being handled.
   */
  void reset(IJsonHandler* pParent, CesiumUtility::ExtensibleObject* pObject);
  /**
   * @brief Reads a property of an \ref CesiumUtility::ExtensibleObject
   * "ExtensibleObject" from the JSON.
   *
   * @param objectType The name of the ExtensibleObject's type.
   * @param str The object key being read.
   * @param o The \ref CesiumUtility::ExtensibleObject "ExtensibleObject" we're
   * currently reading into.
   */
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
  JsonObjectJsonHandler _unknownProperties;
  bool _captureUnknownProperties;
};
} // namespace CesiumJsonReader
