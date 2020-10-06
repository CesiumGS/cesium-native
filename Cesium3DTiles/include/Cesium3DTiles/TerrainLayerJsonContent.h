#pragma once

#include "Cesium3DTiles/TileContent.h"
#include "CesiumUtility/Json.h"
#include "CesiumGeospatial/BoundingRegion.h"
#include <gsl/span>
#include <vector>

namespace Cesium3DTiles {
    
    class TileContext;

    class CESIUM3DTILES_API TerrainLayerJsonContent : public TileContent {
    public:
        static std::string TYPE;

        TerrainLayerJsonContent(TileContext& context, const nlohmann::json& layerJson, const std::string& url);

        virtual const std::string& getType() const { return TerrainLayerJsonContent::TYPE; }
        virtual void finalizeLoad(Tile& tile);

        const CesiumGeospatial::GlobeRectangle& getBounds() const { return this->_bounds; }
        const std::string& getLayerJsonUrl() const { return this->_layerJsonUrl; }
        const std::vector<std::string>& getTilesUrlTemplates() const { return this->_tilesUrlTemplates; }
        const std::string& getVersion() const { return this->_version; }
        const std::vector<std::string>& getExtensions() const { return this->_extensions; }

        virtual void createRasterOverlayTextureCoordinates(
            uint32_t /*textureCoordinateID*/,
            const CesiumGeospatial::Projection& /*projection*/,
            const CesiumGeometry::Rectangle& /*rectangle*/
        ) override {
        }

    private:
        std::vector<Tile> _externalRoot;
        std::vector<std::string> _tilesUrlTemplates;
        std::string _version;
        CesiumGeospatial::GlobeRectangle _bounds;
        std::string _layerJsonUrl;
        std::vector<std::string> _extensions;
    };

}