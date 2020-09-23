#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileRefine.h"
#include <gsl/span>
#include <memory>
#include <vector>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API ExternalTilesetContent {
    public:
        static std::unique_ptr<TileContentLoadResult> load(
            Tileset& tileset,
            const TileID& tileID,
            const BoundingVolume& tileBoundingVolume,
            double tileGeometricError,
            const glm::dmat4& tileTransform,
            const std::optional<BoundingVolume>& tileContentBoundingVolume,
            TileRefine tileRefine,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        );
    };

}
