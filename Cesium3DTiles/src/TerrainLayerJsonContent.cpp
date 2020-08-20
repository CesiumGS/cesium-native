#include "Cesium3DTiles/TerrainLayerJsonContent.h"
#include "CesiumUtility/Json.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Uri.h"

using namespace CesiumGeospatial;

namespace Cesium3DTiles {

    std::string TerrainLayerJsonContent::TYPE = "terrainLayerJson";

    TerrainLayerJsonContent::TerrainLayerJsonContent(
        const Tile& tile,
        const nlohmann::json& layerJson,
        const std::string& url
    ) :
        TileContent(tile),
        _externalRoot(1),
        _tilesUrlTemplates(),
        _version(),
        _bounds(0.0, 0.0, 0.0, 0.0)
    {
        using nlohmann::json;

        this->_externalRoot[0].setTileset(const_cast<Tileset*>(tile.getTileset()));
        this->_externalRoot[0].setParent(const_cast<Tile*>(&tile));

        this->_tilesUrlTemplates = layerJson.value<std::vector<std::string>>("tiles", std::vector<std::string>());
        this->_version = layerJson.value<std::string>("version", "");

        json::const_iterator boundsIt = layerJson.find("bounds");
        if (boundsIt == layerJson.end()) {
        } else {

        }
        std::vector<double> bounds = layerJson.value<std::vector<double>>("bounds", std::vector<double>());
        if (bounds.size() >= 4) {
            this->_bounds = Rectangle(bounds[0], bounds[1], bounds[2], bounds[3]);
        }

        // TODO: use other tile URLs.
        std::string instancedTemplate = Uri::substituteTemplateParameters(this->_tilesUrlTemplates[0], [this](const std::string& placeholder) -> std::string {
            if (placeholder == "z" || placeholder == "x" || placeholder == "y") {
                return "0";
            } else if (placeholder == "version") {
                return this->_version;
            }

            return "";
        });

        this->_externalRoot[0].setContentUri(Uri::resolve(url, instancedTemplate, true));
    }

    void TerrainLayerJsonContent::finalizeLoad(Tile& tile) {
        tile.createChildTiles(std::move(this->_externalRoot));
        tile.setGeometricError(999999999.0);
    }

}
