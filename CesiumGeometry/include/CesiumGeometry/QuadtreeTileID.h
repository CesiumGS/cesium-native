#pragma once

#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {

    struct CESIUMGEOMETRY_API QuadtreeTileID {
        QuadtreeTileID(uint32_t level, uint32_t x, uint32_t y) :
            level(level),
            x(x),
            y(y)
        {
        }

        uint32_t level;
        uint32_t x;
        uint32_t y;
    };

}
