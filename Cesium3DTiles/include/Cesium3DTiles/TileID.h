#pragma once

#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/OctreeTileID.h"
#include <variant>
#include <string>

namespace Cesium3DTiles {

    /**
     * @brief An identifier for a {@link Tile} inside the tile hierarchy.
     * 
     * This ID is stored in the tile as the {@link Tile::getTileID}.
     * Depending on the exact type of the tile and its contents, this
     * identifier may have different forms, is assigned to the
     * tile at construction time, and may be used to identify 
     * and access the children of a given tile.
     */
    typedef std::variant<
        // A URL
        std::string,

        // A tile in an implicit quadtree.
        CesiumGeometry::QuadtreeTileID,

        // A tile in an implicit octree.
        CesiumGeometry::OctreeTileID,

        // One of four tiles created by dividing a parent tile into four parts.
        CesiumGeometry::QuadtreeChild
    > TileID;

}
