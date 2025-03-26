#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>

namespace Cesium3DTilesSelection {

/**
 * @brief A factory to create a tileset loader.
 *
 * This class can be derived to improve the ease of constructing a @ref Tileset
 * from a custom @ref TilesetContentLoader.
 */
class TilesetContentLoaderFactory {
public:
  virtual ~TilesetContentLoaderFactory() = default;

  /**
   * @brief The type of a callback called when the Authorization header used by
   * a tileset loader has changed.
   */
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  /**
   * @brief Creates an instance of the loader corresponding to this factory.
   *
   * @param externals The @ref TilesetExternals.
   * @param tilesetOptions The @ref TilesetOptions.
   * @param headerChangeListener A callback that will be called when the
   * Authorization header used by the tileset loader has changed.
   *
   * @returns A future that resolves to a @ref TilesetContentLoaderResult.
   */
  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) = 0;

  /**
   * @brief Returns true if a valid @ref TilesetContentLoader can be constructed
   * from this factory.
   */
  virtual bool isValid() const = 0;
};
} // namespace Cesium3DTilesSelection