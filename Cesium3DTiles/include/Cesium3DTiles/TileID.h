#pragma once

#include <variant>
#include <string>

namespace Cesium3DTiles {

    struct CESIUM3DTILES_API QuadtreeID {
        QuadtreeID(uint32_t level, uint32_t x, uint32_t y) :
            level(level),
            x(x),
            y(y)
        {
        }

        uint32_t level;
        uint32_t x;
        uint32_t y;
    };

    struct CESIUM3DTILES_API OctreeID {
        OctreeID(uint32_t level, uint32_t x, uint32_t y, uint32_t z) :
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

    typedef std::variant<std::string, QuadtreeID, OctreeID> TileID;

}
