#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "CesiumUtility/Json.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    /*static*/ std::unique_ptr<TileContentLoadResult> ExternalTilesetContent::load(
        const TileContext& context,
        const TileID& /*tileID*/,
        const BoundingVolume& /*tileBoundingVolume*/,
        double /*tileGeometricError*/,
        const glm::dmat4& tileTransform,
        const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
        TileRefine tileRefine,
        const std::string& /*url*/,
        const gsl::span<const uint8_t>& data
    ) {
        std::unique_ptr<TileContentLoadResult> pResult = std::make_unique<TileContentLoadResult>();
        pResult->childTiles.emplace(1);

        using nlohmann::json;

        json tilesetJson = json::parse(data.begin(), data.end());
        context.pTileset->loadTilesFromJson(pResult->childTiles.value()[0], tilesetJson, tileTransform, tileRefine, context);

        return pResult;
    }

}
