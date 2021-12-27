#pragma once

#include "CesiumJsonWriter/JsonWriter.h"
#include "CesiumJsonWriter/Library.h"

#include <any>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>

namespace CesiumJsonWriter {

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

class CESIUMJSONWRITER_API ExtensionWriterContext {
private:
  template <typename TExtension>
  using ExtensionHandler = std::function<
      void(const TExtension&, JsonWriter&, const ExtensionWriterContext&)>;

public:
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
    using TExtension = typename TExtensionHandler::ValueType;

    auto it =
        this->_extensions.emplace(extensionName, ObjectTypeToHandler()).first;
    it->second.insert_or_assign(
        TExtended::TypeName,
        [](const std::any& obj,
           JsonWriter& jsonWriter,
           const ExtensionWriterContext& context) {
          return TExtensionHandler::write(
              std::any_cast<const TExtension&>(obj),
              jsonWriter,
              context);
        });
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
    registerExtension<TExtended, TExtensionHandler>(
        TExtensionHandler::ExtensionName);
  }

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

  ExtensionHandler<std::any> createExtensionHandler(
      const std::string_view& extensionName,
      const std::string& extendedObjectType) const;

private:
  using ObjectTypeToHandler =
      std::unordered_map<std::string, ExtensionHandler<std::any>>;
  using ExtensionNameMap = std::map<std::string, ObjectTypeToHandler>;

  ExtensionNameMap _extensions;
  std::unordered_map<std::string, ExtensionState> _extensionStates;
};

} // namespace CesiumJsonWriter
