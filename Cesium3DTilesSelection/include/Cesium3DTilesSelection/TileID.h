#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <string>
#include <variant>

namespace Cesium3DTilesSelection {

/**
 * @brief An identifier for a {@link Tile} inside the tile hierarchy.
 *
 * This ID is stored in the tile as the {@link Tile::getTileID}.
 * It is assigned to the tile at construction time, and may be
 * used to identify and access the children of a given tile.
 *
 * Depending on the exact type of the tile and its contents, this
 * identifier may have different forms:
 *
 * * A `std::string`: This is an explicitly-described tile and
 *   the ID is the URL of the tile's content.
 * * A {@link CesiumGeometry::QuadtreeTileID}: This is an implicit
 *   tile in the quadtree. The URL of the tile's content is formed
 *   by instantiating the context's template URL with this ID.
 * * A {@link CesiumGeometry::OctreeTileID}: This is an implicit
 *   tile in the octree. The URL of the tile's content is formed
 *   by instantiating the context's template URL with this ID.
 * * A {@link CesiumGeometry::UpsampledQuadtreeNode}: This tile doesn't
 *   have any content, but content for it can be created by subdividing
 *   the parent tile's content.
 */
typedef std::variant<
    std::string,
    CesiumGeometry::QuadtreeTileID,
    CesiumGeometry::OctreeTileID,
    CesiumGeometry::UpsampledQuadtreeNode>
    TileID;

/**
 * @brief Utility functions related to {@link TileID} objects.
 */
struct CESIUM3DTILESSELECTION_API TileIdUtilities {
  /**
   * @brief Creates an unspecified string representation of the given {@link
   * TileID}.
   *
   * The returned string will contain information about the given tile ID,
   * depending on its type. The exact format and contents of this string
   * is not specified. This is mainly intended for printing informative
   * log messages.
   *
   * @param tileId The tile ID
   * @return The string
   */
  static std::string createTileIdString(const TileID& tileId);
};

} // namespace Cesium3DTilesSelection
