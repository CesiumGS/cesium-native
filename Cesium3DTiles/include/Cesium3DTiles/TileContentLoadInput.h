#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetOptions.h"

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTiles {
/**
 * @brief The information that is passed to a {@link TileContentLoader} to
 * create a {@link TileContentLoadResult}.
 *
 * For many types of tile content, only the `data` field is required. The other
 * members are used for content that can generate child tiles, like external
 * tilesets or composite tiles. These members are usually initialized from
 * the corresponding members of the {@link Tile} that the content belongs to.
 */
struct CESIUM3DTILES_API TileContentLoadInput {

  /**
   * @brief Creates a new, uninitialized instance for the given tile.
   *
   * The `data`, `contentType` and `url` will have default values,
   * and have to be initialized before this instance is passed to
   * one of the loader functions.
   *
   * @param pLogger_ The logger that will be used
   * @param tile_ The {@link Tile} that the content belongs to
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger_,
      const Tile& tile_)
      : pLogger(pLogger_),
        data(),
        contentType(),
        url(),
        tileID(tile_.getTileID()),
        tileBoundingVolume(tile_.getBoundingVolume()),
        tileContentBoundingVolume(tile_.getContentBoundingVolume()),
        tileRefine(tile_.getRefine()),
        tileGeometricError(tile_.getGeometricError()),
        tileTransform(tile_.getTransform()),
        contentOptions(
            tile_.getContext()->pTileset->getOptions().contentOptions) {}

  /**
   * @brief Creates a new instance for the given tile.
   *
   * @param pLogger_ The logger that will be used
   * @param data_ The actual data that the tile content will be created from
   * @param contentType_ The content type, if the data was received via a
   * network response
   * @param url_ The URL that the data was loaded from
   * @param tile_ The {@link Tile} that the content belongs to
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger_,
      const gsl::span<const std::byte>& data_,
      const std::string& contentType_,
      const std::string& url_,
      const Tile& tile_)
      : pLogger(pLogger_),
        data(data_),
        contentType(contentType_),
        url(url_),
        tileID(tile_.getTileID()),
        tileBoundingVolume(tile_.getBoundingVolume()),
        tileContentBoundingVolume(tile_.getContentBoundingVolume()),
        tileRefine(tile_.getRefine()),
        tileGeometricError(tile_.getGeometricError()),
        tileTransform(tile_.getTransform()),
        contentOptions(
            tile_.getContext()->pTileset->getOptions().contentOptions) {}

  /**
   * @brief Creates a new instance.
   *
   * For many types of tile content, only the `data` field is required. The
   * other parameters are used for content that can generate child tiles, like
   * external tilesets or composite tiles.
   *
   * @param pLogger_ The logger that will be used
   * @param data_ The actual data that the tile content will be created from
   * @param contentType_ The content type, if the data was received via a
   * network response
   * @param url_ The URL that the data was loaded from
   * @param context_ The {@link TileContext}
   * @param tileID_ The {@link TileID}
   * @param tileBoundingVolume_ The tile {@link BoundingVolume}
   * @param tileContentBoundingVolume_ The tile content {@link BoundingVolume}
   * @param tileRefine_ The {@link TileRefine} strategy
   * @param tileGeometricError_ The geometric error of the tile
   * @param tileTransform_ The tile transform
   */
  TileContentLoadInput(
      const std::shared_ptr<spdlog::logger> pLogger_,
      const gsl::span<const std::byte>& data_,
      const std::string& contentType_,
      const std::string& url_,
      const TileID& tileID_,
      const BoundingVolume& tileBoundingVolume_,
      const std::optional<BoundingVolume>& tileContentBoundingVolume_,
      TileRefine tileRefine_,
      double tileGeometricError_,
      const glm::dmat4& tileTransform_,
      const TilesetContentOptions& contentOptions_)
      : pLogger(pLogger_),
        data(data_),
        contentType(contentType_),
        url(url_),
        tileID(tileID_),
        tileBoundingVolume(tileBoundingVolume_),
        tileContentBoundingVolume(tileContentBoundingVolume_),
        tileRefine(tileRefine_),
        tileGeometricError(tileGeometricError_),
        tileTransform(tileTransform_),
        contentOptions(contentOptions_) {}

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
} // namespace Cesium3DTiles
