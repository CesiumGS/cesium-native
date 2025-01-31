#pragma once

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/Library.h>

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
   * If the extension is a {@link CesiumUtility::JsonValue} or a registered
   * statically-typed class it will be written to the serialized model;
   * otherwise it will be ignored and a warning will be reported.
   */
  Enabled,

  /**
   * @brief The extension is disabled.
   *
   * It will not be represented in the serialized model at all.
   */
  Disabled
};

/**
 * @brief A context for writing extensions where known extensions and their
 * handlers can be registered.
 */
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
   * @tparam TExtensionHandler The extension's writer.
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
   * @tparam TExtensionHandler The extension's writer.
   */
  template <typename TExtended, typename TExtensionHandler>
  void registerExtension() {
    registerExtension<TExtended, TExtensionHandler>(
        TExtensionHandler::ExtensionName);
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
   * By default, all extensions are enabled. However, if the extension is a
   * statically-typed class and is not registered it will be ignored and a
   * warning will be reported.
   *
   * When a disabled extension is encountered, it is ignored completely.
   *
   * @param extensionName The name of the extension to be enabled or disabled.
   * @param newState The new state for the extension.
   */
  void
  setExtensionState(const std::string& extensionName, ExtensionState newState);

  /**
   * @brief Attempts to create an `ExtensionHandler` for the given object,
   * returning `nullptr` if no handler could be found.
   *
   * @param extensionName The name of the extension.
   * @param obj The object of unknown type to create the handler for.
   * @param extendedObjectType The `TypeName` of the extended object.
   * @returns The handler for this extension, or `nullptr` if none could be
   * created.
   */
  ExtensionHandler<std::any> createExtensionHandler(
      const std::string_view& extensionName,
      const std::any& obj,
      const std::string& extendedObjectType) const;

private:
  using ObjectTypeToHandler =
      std::unordered_map<std::string, ExtensionHandler<std::any>>;
  using ExtensionNameMap = std::map<std::string, ObjectTypeToHandler>;

  ExtensionNameMap _extensions;
  std::unordered_map<std::string, ExtensionState> _extensionStates;
};

} // namespace CesiumJsonWriter
