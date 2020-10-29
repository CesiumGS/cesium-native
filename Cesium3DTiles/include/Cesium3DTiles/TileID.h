#pragma once

#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/OctreeTileID.h"
#include <variant>
#include <string>

namespace Cesium3DTiles {

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
