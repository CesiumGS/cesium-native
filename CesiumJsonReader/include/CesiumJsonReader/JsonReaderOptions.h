#pragma once

#include <CesiumJsonReader/IExtensionJsonHandler.h>
#include <CesiumJsonReader/Library.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumJsonReader {

/**
 * @brief The state of an extension.
 */
enum class ExtensionState {
  /**
   * @brief The extension is enabled.
   *
   * If a statically-typed class is available for the extension, it will be
   * used. Otherwise the extension will be represented as a
   * {@link CesiumUtility::JsonValue}.
   */
  Enabled,

  /**
   * @brief The extension is enabled but will always be deserialized as a
   * {@link CesiumUtility::JsonValue}.
   *
   * Even if a statically-typed class is available for the extension, it will
   * not be used.
   */
  JsonOnly,

  /**
   * @brief The extension is disabled.
   *
   * It will not be represented in the loaded model at all.
   */
  Disabled
};

/**
 * @brief Holds options for reading statically-typed data structures from JSON.
 */
class CESIUMJSONREADER_API JsonReaderOptions {
public:
  /**
   * @brief Gets a value indicating whether the values of unknown properties are
   * captured in the {@link CesiumUtility::ExtensibleObject::unknownProperties} field.
   *
   * If this is false, unknown properties are completely ignored.
   */
  bool getCaptureUnknownProperties() const {
    return this->_captureUnknownProperties;
  }

  /**
   * @brief Sets a value indicating whether the values of unknown properties are
   * captured in the {@link CesiumUtility::ExtensibleObject::unknownProperties} field.
   *
   * If this is false, unknown properties are completely ignored.
   */
  void setCaptureUnknownProperties(bool value) {
    this->_captureUnknownProperties = value;
  }

  /**
   * @brief Registers an extension for an object.
   *
   * @tparam TExtended The object to extend.
   * @tparam TExtensionHandler The extension's
   * {@link CesiumJsonReader::JsonHandler}.
   * @param extensionName The name of the extension.
   */
  template <typename TExtended, typename TExtensionHandler>
  void registerExtension(const std::string& extensionName) {
    auto it =
        this->_extensions.emplace(extensionName, ObjectTypeToHandler()).first;
    it->second.insert_or_assign(
        TExtended::TypeName,
        ExtensionReaderFactory([](const JsonReaderOptions& context) {
          return std::make_unique<TExtensionHandler>(context);
        }));
  }

  /**
   * @brief Registers an extension for an object.
   *
   * The extension name is obtained from `TExtensionHandler::ExtensionName`.
   *
   * @tparam TExtended The object to extend.
   * @tparam TExtensionHandler The extension's
   * {@link CesiumJsonReader::JsonHandler}.
   */
  template <typename TExtended, typename TExtensionHandler>
  void registerExtension() {
    auto it =
        this->_extensions
            .emplace(TExtensionHandler::ExtensionName, ObjectTypeToHandler())
            .first;
    it->second.insert_or_assign(
        TExtended::TypeName,
        ExtensionHandlerFactory([](const JsonReaderOptions& context) {
          return std::make_unique<TExtensionHandler>(context);
        }));
  }

  /**
   * @brief Returns whether an extension is enabled or disabled.
   *
   * By default, all extensions are enabled.
   *
   * @param extensionName The name of the extension.
   */
  ExtensionState getExtensionState(const std::string& extensionName) const;

  /**
   * @brief Enables or disables an extension.
   *
   * By default, all extensions are enabled. When an enabled extension is
   * encountered in the source JSON, it is read into a statically-typed
   * extension class, if one is registered, or into a
   * {@link CesiumUtility::JsonValue} if not.
   *
   * When a disabled extension is encountered in the source JSON, it is ignored
   * completely.
   *
   * An extension may also be set to `ExtensionState::JsonOnly`, in which case
   * it will be read into a {@link CesiumUtility::JsonValue} even if a
   * statically-typed extension class is registered.
   *
   * @param extensionName The name of the extension to be enabled or disabled.
   * @param newState The new state for the extension.
   */
  void
  setExtensionState(const std::string& extensionName, ExtensionState newState);

  /**
   * @brief Creates an extension handler for the given extension.
   *
   * @param extensionName The name of the extension to create a handler for.
   * @param extendedObjectType The name of the type of the object that is being
   * extended.
   * @returns An \ref IExtensionJsonHandler to read the extension.
   */
  std::unique_ptr<IExtensionJsonHandler> createExtensionHandler(
      const std::string_view& extensionName,
      const std::string& extendedObjectType) const;

private:
  using ExtensionHandlerFactory =
      std::function<std::unique_ptr<IExtensionJsonHandler>(
          const JsonReaderOptions&)>;
  using ObjectTypeToHandler = std::map<std::string, ExtensionHandlerFactory>;
  using ExtensionNameMap = std::map<std::string, ObjectTypeToHandler>;

  ExtensionNameMap _extensions;
  std::unordered_map<std::string, ExtensionState> _extensionStates;
  bool _captureUnknownProperties = true;
};

} // namespace CesiumJsonReader
