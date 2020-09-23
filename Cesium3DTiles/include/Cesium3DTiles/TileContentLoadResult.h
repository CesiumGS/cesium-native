#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Tile.h"

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
         * An improved bounding volume for this tile, more accurate than the one the tile use originally.
         */
        std::optional<BoundingVolume> updatedBoundingVolume;

        // TODO: loading content can also reveal new tile availability
    };

}
