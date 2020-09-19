#pragma once

#include "Cesium3DTiles/CompleteTileDefinition.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContent.h"
#include "Cesium3DTiles/TileContext.h"
#include <gsl/span>
#include <vector>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API ExternalTilesetContent : public TileContent {
    public:
        static std::string TYPE;

        ExternalTilesetContent(
            const TileContext& tileContext,
            const CompleteTileDefinition& tile,
            const gsl::span<const uint8_t>& data,
            const std::string& url
        );

        virtual const std::string& getType() const { return ExternalTilesetContent::TYPE; }
        virtual void finalizeLoad(Tile& tile);

        virtual void createRasterOverlayTextureCoordinates(
            uint32_t /*textureCoordinateID*/,
            const CesiumGeospatial::Projection& /*projection*/,
            const CesiumGeometry::Rectangle& /*rectangle*/
        ) override {
        }

    private:
        std::vector<Tile> _externalRoot;
    };

}
