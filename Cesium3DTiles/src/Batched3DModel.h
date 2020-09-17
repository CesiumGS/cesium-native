#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include <memory>
#include <string>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API Batched3DModel {
    public:
        static std::string MAGIC;
        static std::unique_ptr<GltfContent> load(
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
