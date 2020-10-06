#pragma once

#include <cstdint>

namespace CesiumGeometry {

    /**
     * A rectangular range of tiles at a particular level of a quadtree.
     */
    struct QuadtreeTileRectangularRange {
        uint32_t level;
        uint32_t minimumX;
        uint32_t minimumY;
        uint32_t maximumX;
        uint32_t maximumY;
    };

}
