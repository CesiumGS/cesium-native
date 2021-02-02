#pragma once

#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {
    class QuadtreeTilingScheme;

    /**
     * @brief Uniquely identifies a node in a quadtree.
     * 
     * This is one form of a {@link Cesium3DTiles::TileID}.
     *
     * The identifier is composed of the level (with 0 being the level of the root tile),
     * the x- and y-coordinate of the tile, referring to a grid coordinate system at
     * the respective level.
     */
    struct CESIUMGEOMETRY_API QuadtreeTileID final {

        /**
         * @brief Creates a new instance.
         * 
         * @param level_ The level of the node, with 0 being the root
         * @param x_ The x-coordinate of the tile 
         * @param y_ The y-coordinate of the tile
         */
        constexpr QuadtreeTileID(uint32_t level_, uint32_t x_, uint32_t y_) noexcept :
            level(level_),
            x(x_),
            y(y_)
        {
        }

        /**
         * @brief Returns `true` if two identifiers are equal.
         */
        constexpr bool operator==(const QuadtreeTileID& other) const noexcept {
            return this->level == other.level && this->x == other.x && this->y == other.y;
        }

        /**
         * @brief Returns `true` if two identifiers are *not* equal.
         */
        constexpr bool operator!=(const QuadtreeTileID& other) const noexcept {
            return !(*this == other);
        }

        /**
         * @brief Computes the inverse y-coordinate of this tile ID.
         * 
         * This will compute the inverse y-coordinate of this tile ID, based 
         * on the given tiling scheme, which provides the number of tiles
         * in y-direction for the level of this tile ID.
         * 
         * @param tilingScheme The {@link QuadtreeTilingScheme}.
         * @return The inverted y-coordinate.
         */
        uint32_t computeInvertedY(const QuadtreeTilingScheme& tilingScheme) const noexcept;

        /**
         * @brief The level of this tile ID, with 0 being the root tile.
         */
        uint32_t level;

        /**
         * @brief The x-coordinate of this tile ID.
         */
        uint32_t x;

        /**
         * @brief The y-coordinate of this tile ID.
         */
        uint32_t y;
    };

    struct CESIUMGEOMETRY_API UpsampledQuadtreeNode final {
        QuadtreeTileID tileID;
    };
}

namespace std {

    /**
     * @brief A hash function for {@link CesiumGeometry::QuadtreeTileID} objects.
     */
    template <>
    struct hash<CesiumGeometry::QuadtreeTileID> {

        /**
         * @brief A specialization of the `std::hash` template for {@link CesiumGeometry::QuadtreeTileID} objects.
         */
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
