#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/BoundingVolume.h"
#include <glm/mat4x4.hpp>
#include <gsl/span>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API GltfContent {
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

        static void createRasterOverlayTextureCoordinates(
            tinygltf::Model& gltf,
            uint32_t textureCoordinateID,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::Rectangle& rectangle
        );
    };

}
