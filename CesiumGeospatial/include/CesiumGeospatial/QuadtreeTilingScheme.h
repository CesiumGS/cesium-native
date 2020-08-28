#pragma once

#include "CesiumGeospatial/Library.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include <optional>
#include <glm/vec2.hpp>

namespace CesiumGeospatial {

    /**
     * @brief Defines how a rectangular region is divided into quadtree tiles.
     */
    class CESIUMGEOSPATIAL_API QuadtreeTilingScheme {
    public:
        /**
         * @brief Constructs a new instance.
         * 
         * @param rectangle The overall rectangle that is tiled, expressed in projected coordinates.
         * @param rootTilesX The number of tiles at the root of the quadtree in the X direction.
         * @param rootTilesY The nubmer of tiles at the root of the quadtree in the Y direction.
         */
        QuadtreeTilingScheme(const CesiumGeometry::Rectangle& rectangle, uint32_t rootTilesX, uint32_t rootTilesY);

        const CesiumGeometry::Rectangle& getRectangle() const { return this->_rectangle; }
        uint32_t getRootTilesX() const { return this->_rootTilesX; }
        uint32_t getRootTilesY() const { return this->_rootTilesY; }

        uint32_t getNumberOfXTilesAtLevel(uint32_t level) const;
        uint32_t getNumberOfYTilesAtLevel(uint32_t level) const;

        std::optional<CesiumGeometry::QuadtreeTileID> positionToTile(const glm::dvec2& position, uint32_t level) const;
        CesiumGeometry::Rectangle tileToRectangle(const CesiumGeometry::QuadtreeTileID& tileID) const;

    private:
        CesiumGeometry::Rectangle _rectangle;
        uint32_t _rootTilesX;
        uint32_t _rootTilesY;
    };

}
