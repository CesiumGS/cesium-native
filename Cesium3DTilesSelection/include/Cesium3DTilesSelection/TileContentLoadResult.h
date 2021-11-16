#pragma once

#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include "CesiumGeospatial/Projection.h"
#include "Tile.h"
#include "TileContext.h"

#include <memory>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief Holds details of the {@link TileContentLoadResult} that are useful
 * for raster overlays.
 */
struct TileContentDetailsForOverlays {
  /**
   * @brief The raster overlay projections for which texture coordinates have
   * been generated.
   *
   * For the projection at index `n`, there is a set of texture coordinates
   * with the attribute name `_CESIUMOVERLAY_n` that corresponds to that
   * projection.
   */
  std::vector<CesiumGeospatial::Projection> rasterOverlayProjections;

  /**
   * @brief The rectangle covered by this tile in each of the
   * {@link rasterOverlayProjections}.
   */
  std::vector<CesiumGeometry::Rectangle> rasterOverlayRectangles;

  /**
   * @brief The precise bounding region of this tile.
   */
  CesiumGeospatial::BoundingRegion boundingRegion;

  /**
   * @brief Finds the rectangle corresponding to a given projection in
   * {@link rasterOverlayProjections}.
   *
   * @param projection The projection.
   * @return The tile's rectangle in the given projection, or nullptr if the
   * projection is not in {@link rasterOverlayProjections}.
   */
  const CesiumGeometry::Rectangle* findRectangleForOverlayProjection(
      const CesiumGeospatial::Projection& projection) const;
};

/**
 * @brief The result of loading a {@link Tile}'s content.
 *
 * The result of loading a tile's content depends on the specific type of
 * content. It can yield a glTF model, a tighter-fitting bounding volume, or
 * knowledge of the availability of tiles deeper in the tile hierarchy. This
 * structure encapsulates all of those possibilities. Each possible result is
 * therefore provided as an `std::optional`.
 *
 * Instances of this structure are created internally, by the
 * {@link TileContentFactory}, when the response to a network request for
 * loading the tile content was received.
 */
struct TileContentLoadResult {
  /**
   * @brief The glTF model to be rendered for this tile.
   *
   * If this is `std::nullopt`, the tile cannot be rendered.
   * If it has a value but the model is blank, the tile can
   * be "rendered", but it is rendered as nothing.
   */
  std::optional<CesiumGltf::Model> model{};

  /**
   * @brief The new contexts used by the `childTiles`, if any.
   */
  std::vector<std::unique_ptr<TileContext>> newTileContexts;

  /**
   * @brief New child tiles discovered by loading this tile.
   *
   * For example, if the content is an external tileset, this property
   * contains the root tiles of the subtree. This is ignored if the
   * tile already has any child tiles.
   */
  std::optional<std::vector<Tile>> childTiles{};

  /**
   * @brief An improved bounding volume for this tile.
   *
   * If this is available, then it is more accurate than the one the tile used
   * originally.
   */
  std::optional<BoundingVolume> updatedBoundingVolume{};

  /**
   * @brief An improved bounding volume for the content of this tile.
   *
   * If this is available, then it is more accurate than the one the tile used
   * originally.
   */
  std::optional<BoundingVolume> updatedContentBoundingVolume{};

  /**
   * @brief Available quadtree tiles discovered as a result of loading this
   * tile.
   */
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange>
      availableTileRectangles{};

  /**
   * @brief The HTTP status code received when accessing this content.
   */
  uint16_t httpStatusCode = 0;

  /**
   * @brief Holds details of this content that are useful for raster overlays.
   *
   * If this tile does not have any overlays, this field will be std::nullopt.
   */
  std::optional<TileContentDetailsForOverlays> overlayDetails;
};

} // namespace Cesium3DTilesSelection
