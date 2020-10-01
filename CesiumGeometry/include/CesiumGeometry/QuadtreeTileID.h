#pragma once

#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {
    class QuadtreeTilingScheme;

    struct CESIUMGEOMETRY_API QuadtreeTileID {
        QuadtreeTileID(uint32_t level_, uint32_t x_, uint32_t y_) :
            level(level_),
            x(x_),
            y(y_)
        {
        }

        bool operator==(const QuadtreeTileID& other) const {
            return this->level == other.level && this->x == other.x && this->y == other.y;
        }

        bool operator!=(const QuadtreeTileID& other) const {
            return this->level != other.level || this->x != other.x || this->y != other.y;
        }

        uint32_t computeInvertedY(const QuadtreeTilingScheme& tilingScheme) const;

        uint32_t level;
        uint32_t x;
        uint32_t y;
    };

}

namespace std {
    template <>
    struct hash<CesiumGeometry::QuadtreeTileID> {
        size_t operator()(const CesiumGeometry::QuadtreeTileID& key) const noexcept {
            // TODO: is this hash function any good? Probably not.
            std::hash<uint32_t> h;
            return
                h(key.level) ^
                (h(key.x) << 1) ^
                (h(key.y) << 2);
        }
    };
}
