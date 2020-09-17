#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "CesiumUtility/Json.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    std::string ExternalTilesetContent::TYPE = "json";

    ExternalTilesetContent::ExternalTilesetContent(
        Tileset& tileset,
        const glm::dmat4& tileTransform,
        TileRefine tileRefine,
        const gsl::span<const uint8_t>& data,
        const std::string& /*url*/
    ) :
        TileContent(),
        _externalRoot(1)
    {
        this->_externalRoot[0].setTransform(tileTransform);

        using nlohmann::json;

        json tilesetJson = json::parse(data.begin(), data.end());
        tileset.loadTilesFromJson(this->_externalRoot[0], tilesetJson, tileTransform, tileRefine);
    }

    void ExternalTilesetContent::finalizeLoad(Tile& tile) {
        this->_externalRoot[0].setParent(&tile);
        tile.createChildTiles(std::move(this->_externalRoot));
        tile.setGeometricError(999999999.0);
    }

}
