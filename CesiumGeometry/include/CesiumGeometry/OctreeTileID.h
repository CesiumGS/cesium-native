#pragma once

#include "CesiumGeometry/OctreeTileID.h"

namespace CesiumGeometry {

    struct CESIUMGEOMETRY_API OctreeTileID {
        OctreeTileID(uint32_t level, uint32_t x, uint32_t y, uint32_t z) :
            level(level),
            x(x),
            y(y),
            z(z)
        {
        }

        uint32_t level;
        uint32_t x;
        uint32_t y;
        uint32_t z;
    };

}
