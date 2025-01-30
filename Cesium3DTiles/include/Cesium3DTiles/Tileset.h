#pragma once

#include <Cesium3DTiles/Library.h>
#include <Cesium3DTiles/TilesetSpec.h>

#include <glm/mat4x4.hpp>

#include <functional>

namespace Cesium3DTiles {

/** @copydoc TilesetSpec */
struct CESIUM3DTILES_API Tileset : public TilesetSpec {
  Tileset() = default;

  /**
   * @brief A callback function for {@link forEachTile}.
   */
  typedef void ForEachTileCallback(
      Tileset& tileset,
      Tile& tile,
      const glm::dmat4& transform);

  /**
   * @brief Apply the given callback to all tiles.
   *
   * This will only invoke the callback for explicit tiles. It won't follow
   * external tilesets or implicit tiles.
   *
   * @param callback The callback to apply
   */
  void forEachTile(std::function<ForEachTileCallback>&& callback);

  /**
   * @brief A callback function for {@link forEachTile}.
   */
  typedef void ForEachTileConstCallback(
      const Tileset& tileset,
      const Tile& tile,
      const glm::dmat4& transform);

  /** @copydoc Tileset::forEachTile */
  void forEachTile(std::function<ForEachTileConstCallback>&& callback) const;

  /**
   * @brief A callback function for {@link forEachContent}.
   */
  typedef void ForEachContentCallback(
      Tileset& tileset,
      Tile& tile,
      Content& content,
      const glm::dmat4& transform);

  /**
   * @brief Apply the given callback to all contents.
   *
   * This will only invoke the callback for contents of explicit tiles. It won't
   * follow external tilesets or implicit tiles.
   *
   * @param callback The callback to apply
   */
  void forEachContent(std::function<ForEachContentCallback>&& callback);

  /**
   * @brief A callback function for {@link forEachContent}.
   */
  typedef void ForEachContentConstCallback(
      const Tileset& tileset,
      const Tile& tile,
      const Content& content,
      const glm::dmat4& transform);

  /** @copydoc Tileset::forEachContent */
  void
  forEachContent(std::function<ForEachContentConstCallback>&& callback) const;

  /**
   * @brief Adds an extension to the {@link TilesetSpec::extensionsUsed}
   * property, if it is not already present.
   *
   * @param extensionName The name of the used extension.
   */
  void addExtensionUsed(const std::string& extensionName);

  /**
   * @brief Adds an extension to the {@link TilesetSpec::extensionsRequired}
   * property, if it is not already present.
   *
   * Calling this function also adds the extension to `extensionsUsed`, if it's
   * not already present.
   *
   * @param extensionName The name of the required extension.
   */
  void addExtensionRequired(const std::string& extensionName);

  /**
   * @brief Removes an extension from the {@link TilesetSpec::extensionsUsed}
   * property.
   *
   * @param extensionName The name of the used extension.
   */
  void removeExtensionUsed(const std::string& extensionName);

  /**
   * @brief Removes an extension from the {@link TilesetSpec::extensionsRequired}
   * property.
   *
   * Calling this function also removes the extension from `extensionsUsed`.
   *
   * @param extensionName The name of the required extension.
   */
  void removeExtensionRequired(const std::string& extensionName);

  /**
   * @brief Determines whether a given extension name is listed in the tileset's
   * {@link TilesetSpec::extensionsUsed} property.
   *
   * @param extensionName The extension name to check.
   * @returns True if the extension is found in `extensionsUsed`; otherwise,
   * false.
   */
  bool isExtensionUsed(const std::string& extensionName) const noexcept;

  /**
   * @brief Determines whether a given extension name is listed in the tileset's
   * {@link TilesetSpec::extensionsRequired} property.
   *
   * @param extensionName The extension name to check.
   * @returns True if the extension is found in `extensionsRequired`; otherwise,
   * false.
   */
  bool isExtensionRequired(const std::string& extensionName) const noexcept;
};

} // namespace Cesium3DTiles
