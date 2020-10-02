#pragma once

#include "CesiumGeometry/Library.h"
#include "CesiumGeometry/QuadtreeTileRectangularRange.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeometry/Rectangle.h"
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

namespace CesiumGeometry {

    class CESIUMGEOMETRY_API QuadtreeTileAvailability {
    public:
        QuadtreeTileAvailability(const QuadtreeTilingScheme& tilingScheme, uint32_t maximumLevel);

        void addAvailableTileRange(const QuadtreeTileRectangularRange& range);
        uint32_t computeMaximumLevelAtPosition(const glm::dvec2& position) const;

    private:
        struct RectangleWithLevel {
            uint32_t level;
            Rectangle rectangle;
        };

        struct QuadtreeNode {
            QuadtreeNode(const QuadtreeTileID& id_, const Rectangle& extent_) :
                id(id_),
                extent(extent_)
            {}

            QuadtreeTileID id;

            CesiumGeometry::Rectangle extent;

            QuadtreeNode* pParent;
            std::unique_ptr<QuadtreeNode> ll;
            std::unique_ptr<QuadtreeNode> lr;
            std::unique_ptr<QuadtreeNode> ul;
            std::unique_ptr<QuadtreeNode> ur;

            std::vector<RectangleWithLevel> rectangles;
        };

        QuadtreeTilingScheme _tilingScheme;
        uint32_t _maximumLevel;
        std::vector<std::unique_ptr<QuadtreeNode>> _rootNodes;

        static void putRectangleInQuadtree(const QuadtreeTilingScheme& tilingScheme, uint32_t maximumLevel, QuadtreeTileAvailability::QuadtreeNode& node, const QuadtreeTileAvailability::RectangleWithLevel& rectangle);
        static bool rectangleLevelComparator(const QuadtreeTileAvailability::RectangleWithLevel& a, const QuadtreeTileAvailability::RectangleWithLevel& b);
        static uint32_t findMaxLevelFromNode(QuadtreeNode* pStopNode, QuadtreeNode& node, const glm::dvec2& position);
        static void createNodeChildrenIfNecessary(QuadtreeNode& node, const QuadtreeTilingScheme& tilingScheme);
    };
}
