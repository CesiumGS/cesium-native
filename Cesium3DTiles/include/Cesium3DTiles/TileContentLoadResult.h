// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContext.h"
#include "CesiumGeometry/QuadtreeTileRectangularRange.h"

namespace Cesium3DTiles {

    class Tile;

    /**
     * @brief The result of loading a {@link Tile}'s content.
     * 
     * The result of loading a tile's content depends on the specific type of content.
     * It can yield a glTF model, a tighter-fitting bounding volume, or knowledge of 
     * the availability of tiles deeper in the tile hierarchy. This structure 
     * encapsulates all of those possibilities. Each possible result is therefore
     * provided as an `std::optional`.
     * 
     * Instances of this structure are created internally, by the {@link TileContentFactory},
     * when the response to a network request for loading the tile content was
     * received.
     */
    struct TileContentLoadResult {
        /**
         * @brief The glTF model to be rendered for this tile. 
         *
         * If this is `std::nullopt`, the tile cannot be rendered.
         * If it has a value but the model is blank, the tile can 
         * be "rendered", but it is rendered as nothing.
         */
        std::optional<CesiumGltf::Model> model;

        /**
         * @brief A new context, if any, used by the `childTiles`.
         */
        std::unique_ptr<TileContext> pNewTileContext;

        /**
         * @brief New child tiles discovered by loading this tile. 
         * 
         * For example, if the content is an external tileset, this property
         * contains the root tiles of the subtree. This is ignored if the 
         * tile already has any child tiles.
         */
        std::optional<std::vector<Tile>> childTiles;

        /**
         * @brief An improved bounding volume for this tile.
         *
         * If this is available, then it is more accurate than the one the tile used originally.
         */
        std::optional<BoundingVolume> updatedBoundingVolume;

        /**
         * @brief Available quadtree tiles discovered as a result of loading this tile.
         */
        std::vector<CesiumGeometry::QuadtreeTileRectangularRange> availableTileRectangles;

        /**
         * @brief The HTTP status code received when accessing this content.
         */
        uint16_t httpStatusCode;

        // TODO: other forms of tile availability, like a bitfield?
    };

}
