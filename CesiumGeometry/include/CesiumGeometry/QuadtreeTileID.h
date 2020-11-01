#pragma once

#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {
    class QuadtreeTilingScheme;

    /**
     * @brief Enumeration of identifiers for the four children of a quadtree node.
     */
    enum class QuadtreeChild {

        /**
         * @brief The lower left child node.
         */
        LowerLeft = 0,

        /**
         * @brief The lower right child node.
         */
        LowerRight = 1,

        /**
         * @brief The upper left child node.
         */
        UpperLeft = 2,

        /**
         * @brief The upper right child node.
         */
        UpperRight = 3
    };

    /**
     * @brief A structure serving as a unique identifier for a node in a quadtree.
     * 
     * This is one form of a {@link Cesium3DTiles::TileID}.
     *
     * The identifier is composed of the level (with 0 being the level of the root tile),
     * the x- and y-coordinate of the tile, referring to a grid coordinate system at
     * the respective level.
     */
    struct CESIUMGEOMETRY_API QuadtreeTileID {

        /**
         * @brief Creates a new instance.
         * 
         * @param level_ The level of the node, with 0 being the root
         * @param x_ The x-coordinate of the tile 
         * @param y_ The y-coordinate of the tile
         */
        QuadtreeTileID(uint32_t level_, uint32_t x_, uint32_t y_) :
            level(level_),
            x(x_),
            y(y_)
        {
        }

        /**
         * @brief Returns `true` if two identifiers are equal.
         */
        bool operator==(const QuadtreeTileID& other) const {
            return this->level == other.level && this->x == other.x && this->y == other.y;
        }

        /**
         * @brief Returns `true` if two identifiers are *not* equal.
         */
        bool operator!=(const QuadtreeTileID& other) const {
            return this->level != other.level || this->x != other.x || this->y != other.y;
        }

        /**
         * @brief Computes the invese y-coordinate of this tile ID.
         * 
         * This will compute the invese y-coordinate of this tile ID, based 
         * on the given tiling scheme, which provides the number of tiles
         * in y-direction for the level of this tile ID.
         * 
         * @param tilingScheme The {@link QuadtreeTilingScheme}.
         * @return The inverted y-coordinate.
         */
        uint32_t computeInvertedY(const QuadtreeTilingScheme& tilingScheme) const;

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
