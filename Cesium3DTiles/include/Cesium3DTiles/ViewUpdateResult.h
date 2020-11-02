#pragma once

#include <vector>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {
    class Tile;

    struct CESIUM3DTILES_API ViewUpdateResult {
        std::vector<Tile*> tilesToRenderThisFrame;
        std::vector<Tile*> tilesToNoLongerRenderThisFrame;

        uint32_t tilesLoadingLowPriority;
        uint32_t tilesLoadingMediumPriority;
        uint32_t tilesLoadingHighPriority;

        uint32_t tilesVisited;
        uint32_t tilesCulled;
        uint32_t maxDepthVisited;
    };

}
