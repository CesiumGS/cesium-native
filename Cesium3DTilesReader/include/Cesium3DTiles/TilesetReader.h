#pragma once

#include "Cesium3DTiles/IExtensionJsonHandler.h"
#include "Cesium3DTiles/ReaderLibrary.h"
#include "Cesium3DTiles/Tileset.h"

#include <gsl/span>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Cesium3DTiles {

struct ReaderContext;

/**
 * @brief The result of reading a tileset with
 * {@link TilesetReader::readTileset}.
 */
struct CESIUM3DTILESREADER_API TilesetReaderResult {
  /**
   * @brief The read tileset, or std::nullopt if the tileset could not be read.
   */
  std::optional<Tileset> tileset;

  /**
   * @brief Errors, if any, that occurred during the load process.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings, if any, that occurred during the load process.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief The state of a tileset extension.
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
   * It will not be represented in the loaded tileset at all.
   */
  Disabled
};

/**
 * @brief Options for how to read a tileset.
 */
struct CESIUM3DTILESREADER_API ReadTilesetOptions {
  /**
   * @brief Test.
   */
  bool test = true;
};

/**
 * @brief Reads tilesets.
 */
class CESIUM3DTILESREADER_API TilesetReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  TilesetReader();

  /**
   * @brief Registers an extension for a tileset object.
   *
   * @tparam TExtended The tileset object to extend.
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
        ExtensionReaderFactory([](const ReaderContext& context) {
          return std::make_unique<TExtensionHandler>(context);
        }));
  }

  /**
   * @brief Registers an extension for a tileset object.
   *
   * The extension name is obtained from `TExtensionHandler::ExtensionName`.
   *
   * @tparam TExtended The tileset object to extend.
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
        ExtensionHandlerFactory([](const ReaderContext& context) {
          return std::make_unique<TExtensionHandler>(context);
        }));
  }

  /**
   * @brief Enables or disables a tileset extension.
   *
   * By default, all extensions are enabled. When an enabled extension is
   * encountered in the source tileset, it is read into a statically-typed
   * extension class, if one is registered, or into a
   * {@link CesiumUtility::JsonValue} if not.
   *
   * When a disabled extension is encountered in the source tileset, it is
   * ignored completely.
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
   * @brief Reads a tileset.
   *
   * @param data The buffer from which to read the tileset.
   * @param options Options for how to read the tileset.
   * @return The result of reading the tileset.
   */
  TilesetReaderResult readTileset(
      const gsl::span<const std::byte>& data,
      const ReadTilesetOptions& options = ReadTilesetOptions()) const;

  std::unique_ptr<IExtensionJsonHandler> createExtensionHandler(
      const ReaderContext& context,
      const std::string_view& extensionName,
      const std::string& extendedObjectType) const;

private:
  using ExtensionHandlerFactory =
      std::function<std::unique_ptr<IExtensionJsonHandler>(
          const ReaderContext&)>;
  using ObjectTypeToHandler = std::map<std::string, ExtensionHandlerFactory>;
  using ExtensionNameMap = std::map<std::string, ObjectTypeToHandler>;

  ExtensionNameMap _extensions;
  std::unordered_map<std::string, ExtensionState> _extensionStates;
};

} // namespace Cesium3DTiles
