#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "CesiumUtility/Json.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    std::string ExternalTilesetContent::TYPE = "json";

    ExternalTilesetContent::ExternalTilesetContent(
        const TileContext& tileContext,
        const CompleteTileDefinition& tile,
        const gsl::span<const uint8_t>& data,
        const std::string& url
    ) :
        TileContent(),
        _externalRoot(1)
    {
        using nlohmann::json;

        json tilesetJson = json::parse(data.begin(), data.end());
        tileContext.pTileset->loadTilesFromJson(this->_externalRoot[0], tilesetJson, tile.transform, tile.refine, url);
    }

    void ExternalTilesetContent::finalizeLoad(Tile& tile) {
        this->_externalRoot[0].setParent(&tile);
        tile.createChildTiles(std::move(this->_externalRoot));
        tile.setGeometricError(999999999.0);
    }

}
