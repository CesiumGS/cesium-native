#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Batched3DModel.h"
#include "QuantizedMeshContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerMagic(GltfContent::TYPE, [](
            Tileset& /*tileset*/,
            const TileID& /*tileID*/,
            const BoundingVolume& /*tileBoundingVolume*/,
            double /*tileGeometricError*/,
            const glm::dmat4& /*tileTransform*/,
            const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
            TileRefine /*tileRefine*/,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<GltfContent>(data, url);
        });

        TileContentFactory::registerMagic(ExternalTilesetContent::TYPE, [](
            Tileset& tileset,
            const TileID& /*tileID*/,
            const BoundingVolume& /*tileBoundingVolume*/,
            double /*tileGeometricError*/,
            const glm::dmat4& tileTransform,
            const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
            TileRefine tileRefine,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<ExternalTilesetContent>(tileset, tileTransform, tileRefine, data, url);
        });

        TileContentFactory::registerMagic(Batched3DModel::MAGIC, Batched3DModel::load);

        TileContentFactory::registerContentType(QuantizedMeshContent::CONTENT_TYPE, [](
            Tileset& /*tileset*/,
            const TileID& /*tileID*/,
            const BoundingVolume& tileBoundingVolume,
            double /*tileGeometricError*/,
            const glm::dmat4& /*tileTransform*/,
            const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
            TileRefine /*tileRefine*/,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<QuantizedMeshContent>(tileBoundingVolume, data, url);
        });
    }

}
