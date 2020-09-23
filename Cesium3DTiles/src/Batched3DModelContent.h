#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include <memory>
#include <string>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API Batched3DModelContent {
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
