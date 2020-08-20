#pragma once

#include "Cesium3DTiles/TileContent.h"
#include "CesiumUtility/Json.h"
#include "CesiumGeospatial/BoundingRegion.h"
#include <gsl/span>
#include <vector>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API TerrainLayerJsonContent : public TileContent {
    public:
        static std::string TYPE;

        TerrainLayerJsonContent(const Tile& tile, const nlohmann::json& layerJson, const std::string& url);

        virtual const std::string& getType() const { return TerrainLayerJsonContent::TYPE; }
        virtual void finalizeLoad(Tile& tile);

        const CesiumGeospatial::Rectangle& getBounds() const { return this->_bounds; }

    private:
        std::vector<Tile> _externalRoot;
        std::vector<std::string> _tilesUrlTemplates;
        std::string _version;
        CesiumGeospatial::Rectangle _bounds;
    };

}