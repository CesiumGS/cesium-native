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

        /**
         * @brief Creates texture coordinates for raster tiles that are mapped to 3D tiles.
         * 
         * This is not supposed to be called by clients.
         * 
         * It will be called for all {@link RasterMappedTo3DTile} objects of a {@link Tile},
         * and extend the accessors of the given glTF model with accessors that contain the 
         * texture coordinate sets for different projections. Further details are not
         * specified here.
         * 
         * @param gltf The glTF model.
         * @param textureCoordinateID The texture coordinate ID.
         * @param projection The {@link CesiumGeospatial::Projection}.
         * @param rectangle The {@link CesiumGeometry::Rectangle}.
         * @return The bounding region.
         */
        static CesiumGeospatial::BoundingRegion createRasterOverlayTextureCoordinates(
            tinygltf::Model& gltf,
            uint32_t textureCoordinateID,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::Rectangle& rectangle
        );
    };

}
