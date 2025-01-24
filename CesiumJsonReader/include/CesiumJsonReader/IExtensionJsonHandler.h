#pragma once

#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <any>
#include <string_view>

namespace CesiumJsonReader {

/**
 * @brief An interface for JSON handlers that handle extensions on \ref
 * CesiumUtility::ExtensibleObject "ExtensibleObject" types.
 *
 * Usually, this will be a handler deriving from \ref ExtensionsJsonHandler that
 * handles the specific extension found in the JSON. However, unknown extensions
 * are read by a special \ref IExtensionJsonHandler returned by \ref
 * JsonReaderOptions::createExtensionHandler, which reads the extension into the
 * ExtensibleObject's `extensions` map as a \ref CesiumUtility::JsonValue
 * "JsonValue".
 */
class IExtensionJsonHandler {
public:
  virtual ~IExtensionJsonHandler() noexcept = default;
  /**
   * @brief Resets this \ref IExtensionJsonHandler's parent handler, destination
   * object, and extension name.
   */
  virtual void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
  /**
   * @brief Obtains an \ref IJsonHandler from this \ref IExtensionJsonHandler.
   */
  virtual IJsonHandler& getHandler() = 0;
};

} // namespace CesiumJsonReader
