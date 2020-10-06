#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContext.h"
#include "CesiumGeometry/QuadtreeTileRectangularRange.h"

namespace Cesium3DTiles {

    class Tile;

    struct TileContentLoadResult {
        /**
         * The glTF model to be rendered for this tile. If this is `std::nullopt`, the tile cannot be rendered.
         * If it has a value but the model is blank, the tile can be "rendered", but it is rendered as nothing.
         */
        std::optional<tinygltf::Model> model;

        /**
         * New child tiles discovered by loading this tile. For example, if the content is an external tileset, this property
         * contains the root tiles of the subtree. This is ignored if the tile already has any child tiles.
         */
        std::optional<std::vector<Tile>> childTiles;

        /**
         * A new context, if any, used by the `childTiles`.
         */
        std::unique_ptr<TileContext> pNewTileContext;

        /**
         * An improved bounding volume for this tile, more accurate than the one the tile used originally.
         */
        std::optional<BoundingVolume> updatedBoundingVolume;

        /**
         * Available quadtree tiles discovered as a result of loading this tile.
         */
        std::vector<CesiumGeometry::QuadtreeTileRectangularRange> availableTileRectangles;

        // TODO: other forms of tile availability, like a bitfield?
    };

}
