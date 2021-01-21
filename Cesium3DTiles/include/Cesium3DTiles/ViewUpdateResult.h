#pragma once

#include "Cesium3DTiles/Library.h"
#include <vector>

namespace Cesium3DTiles {
    class Tile;

    /**
     * @brief Reports the results of {@link Tileset::updateView}.
     * 
     * Users of a {@link Tileset} will call {@link Tileset::updateView} and receive
     * this structure so that they can update the state of their rendering system
     * accordingly. The tileset will internally keep track the current state of the 
     * tiles as their {@link Tile::getLastSelectionState} throughout the rendering 
     * process, and use this structure to provide information about the state
     * changes of tiles to clients.
     */
    class CESIUM3DTILES_API ViewUpdateResult final {
    public:

        /**
         * @brief The tiles that are contained in the render list of the current frame.
         */
        std::vector<Tile*> tilesToRenderThisFrame;

        /**
         * @brief The tiles that have been removed from the render list for the current frame
         */
        std::vector<Tile*> tilesToNoLongerRenderThisFrame;

        //! @cond Doxygen_Suppress
        uint32_t tilesLoadingLowPriority = 0;
        uint32_t tilesLoadingMediumPriority = 0;
        uint32_t tilesLoadingHighPriority = 0;

        uint32_t tilesVisited = 0;
        uint32_t culledTilesVisited = 0;
        uint32_t tilesCulled = 0;
        uint32_t maxDepthVisited = 0;
        //! @endcond
    };

}
