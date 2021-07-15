#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/TileContext.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TileRefine.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTilesSelection {
/**
 * @brief The information that is passed to a {@link TileContentLoader} to
 * create a {@link TileContentLoadResult}.
 *
 * For many types of tile content, only the `data` field is required. The other
 * members are used for content that can generate child tiles, like external
 * tilesets or composite tiles. These members are usually initialized from
 * the corresponding members of the {@link Tile} that the content belongs to.
 */
struct CESIUM3DTILESSELECTION_API TileContentLoadInput {

  /**
   * @brief Creates a new, uninitialized instance for the given tile.
   *
   * The `data`, `contentType` and `url` will have default values,
   * and have to be initialized before this instance is passed to
   * one of the loader functions.
   *
   * @param pLogger The logger that will be used
   * @param tile The {@link Tile} that the content belongs to
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger,
      const Tile& tile);

  /**
   * @brief Creates a new instance for the given tile.
   *
   * @param pLogger The logger that will be used
   * @param data The actual data that the tile content will be created from
   * @param contentType The content type, if the data was received via a
   * network response
   * @param url The URL that the data was loaded from
   * @param tile The {@link Tile} that the content belongs to
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger,
      const gsl::span<const std::byte>& data,
      const std::string& contentType,
      const std::string& url,
      const Tile& tile);

  /**
   * @brief Creates a new instance.
   *
   * For many types of tile content, only the `data` field is required. The
   * other parameters are used for content that can generate child tiles, like
   * external tilesets or composite tiles.
   *
   * @param pLogger The logger that will be used
   * @param data The actual data that the tile content will be created from
   * @param contentType The content type, if the data was received via a
   * network response
   * @param url The URL that the data was loaded from
   * @param tileID The {@link TileID}
   * @param tileBoundingVolume The tile {@link BoundingVolume}
   * @param tileContentBoundingVolume The tile content {@link BoundingVolume}
   * @param tileRefine The {@link TileRefine} strategy
   * @param tileGeometricError The geometric error of the tile
   * @param tileTransform The tile transform
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger,
      const gsl::span<const std::byte>& data,
      const std::string& contentType,
      const std::string& url,
      const TileID& tileID,
      const BoundingVolume& tileBoundingVolume,
      const std::optional<BoundingVolume>& tileContentBoundingVolume,
      TileRefine tileRefine,
      double tileGeometricError,
      const glm::dmat4& tileTransform,
      const TilesetContentOptions& contentOptions);

  /**
   * @brief The logger that receives details of loading errors and warnings.
   */
  std::shared_ptr<spdlog::logger> pLogger;

  /**
   * @brief The raw input data.
   *
   * The {@link TileContentFactory} will try to determine the type of the
   * data using the first four bytes (i.e. the "magic header"). If this
   * does not succeed, it will try to determine the type based on the
   * `contentType` field.
   */
  gsl::span<const std::byte> data;

  /**
   * @brief The content type.
   *
   * If the data was obtained via a HTTP response, then this will be
   * the `Content-Type` of that response. The {@link TileContentFactory}
   * will try to interpret the data based on this content type.
   *
   * If the data was not directly obtained from an HTTP response, then
   * this may be the empty string.
   */
  std::string contentType;

  /**
   * @brief The source URL.
   */
  std::string url;

  /**
   * @brief The {@link TileID}.
   */
  TileID tileID;

  /**
   * @brief The tile {@link BoundingVolume}.
   */
  BoundingVolume tileBoundingVolume;

  /**
   * @brief Tile content {@link BoundingVolume}.
   */
  std::optional<BoundingVolume> tileContentBoundingVolume;

  /**
   * @brief The {@link TileRefine}.
   */
  TileRefine tileRefine;

  /**
   * @brief The geometric error.
   */
  double tileGeometricError;

  /**
   * @brief The tile transform
   */
  glm::dmat4 tileTransform;

  /**
   * @brief Options for parsing content and creating Gltf models.
   */
  TilesetContentOptions contentOptions;
};
} // namespace Cesium3DTilesSelection
