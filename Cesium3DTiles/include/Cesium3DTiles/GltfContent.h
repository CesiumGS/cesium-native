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

    /**
     * @brief Creates {@link TileContentLoadResult} from glTF data.
     */
    class CESIUM3DTILES_API GltfContent {
    public:
        /** @copydoc ExternalTilesetContent::load */
        static std::unique_ptr<TileContentLoadResult> load(
            const TileContext& context,
            const TileID& tileID,
            const BoundingVolume& tileBoundingVolume,
            double tileGeometricError,
            const glm::dmat4& tileTransform,
            const std::optional<BoundingVolume>& tileContentBoundingVolume,
            TileRefine tileRefine,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        );

        static CesiumGeospatial::BoundingRegion createRasterOverlayTextureCoordinates(
            tinygltf::Model& gltf,
            uint32_t textureCoordinateID,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::Rectangle& rectangle
        );
    };

}
