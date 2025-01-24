#pragma once

#include <CesiumJsonReader/IExtensionJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <memory>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading extensions, such as those listed in an
 * \ref CesiumUtility::ExtensibleObject "ExtensibleObject".
 */
class ExtensionsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  /**
   * @brief Creates a new \ref ExtensionsJsonHandler with the specified reader
   * options.
   *
   * @param context Options to configure how the JSON is read.
   */
  explicit ExtensionsJsonHandler(const JsonReaderOptions& context) noexcept
      : ObjectJsonHandler(),
        _context(context),
        _pObject(nullptr),
        _currentExtensionHandler() {}

  /**
   * @brief Resets the \ref IJsonHandler's parent and pointer to destination, as
   * well as the name of the object type that the extension is attached to.
   */
  void reset(
      IJsonHandler* pParent,
      CesiumUtility::ExtensibleObject* pObject,
      const std::string& objectType);

  /** @copydoc IJsonHandler::readObjectKey */
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

private:
  const JsonReaderOptions& _context;
  CesiumUtility::ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumJsonReader
