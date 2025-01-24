#pragma once

#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

#include <string_view>

namespace CesiumJsonReader {

/**
 * @brief \ref IJsonHandler for \ref CesiumUtility::SharedAsset "SharedAsset"
 * values.
 *
 * This is more or less the same as directly using \ref
 * ExtensibleObjectJsonHandler, and exists for compatibility with generated
 * code.
 */
class SharedAssetJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  /**
   * @brief Creates an \ref SharedAssetJsonHandler with the specified
   * options.
   *
   * @param context Options to configure how the JSON is parsed.
   */
  explicit SharedAssetJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& context) noexcept;

protected:
  /** @copydoc ExtensibleObjectJsonHandler::reset */
  void reset(IJsonHandler* pParent, CesiumUtility::ExtensibleObject* pObject);
  /** @copydoc ExtensibleObjectJsonHandler::readObjectKeyExtensibleObject */
  IJsonHandler* readObjectKeySharedAsset(
      const std::string& objectType,
      const std::string_view& str,
      CesiumUtility::ExtensibleObject& o);

private:
};
} // namespace CesiumJsonReader
