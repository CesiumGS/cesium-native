#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include <glm/mat4x4.hpp>
#include <optional>

namespace Cesium3DTiles {

    struct CompleteTileDefinition {
        TileID id;
        BoundingVolume boundingVolume;
        std::optional<BoundingVolume> viewerRequestVolume;
        double geometricError;
        TileRefine refine;
        glm::dmat4 transform;
        std::optional<BoundingVolume> contentBoundingVolume;
    };

}