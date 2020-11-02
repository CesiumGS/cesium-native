#pragma once

#include <vector>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {
    class Tile;

    /**
     * @brief A structure summarizing the results of {@link Tileset::updateView}.
     * 
     * This is not supposed to be used by clients. It is used for the internal 
     * bookkeeping, and to update the {@link Tile::getLastSelectionState} of the
     * tiles throughout the rendering process.
     */
    class CESIUM3DTILES_API ViewUpdateResult {
    public:

        /**
         * @brief The tiles that are contained in the render list of the current frame
         */
        std::vector<Tile*> tilesToRenderThisFrame;

        /**
         * @brief The tiles that have been removed from the render list for the current frame
         */
        std::vector<Tile*> tilesToNoLongerRenderThisFrame;

        uint32_t tilesLoadingLowPriority;
        uint32_t tilesLoadingMediumPriority;
        uint32_t tilesLoadingHighPriority;

        uint32_t tilesVisited;
        uint32_t tilesCulled;
        uint32_t maxDepthVisited;
    };

}
