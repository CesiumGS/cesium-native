#pragma once

#include <string>
#include "Cesium3DTiles/Library.h"
#include "CesiumGeospatial/Projection.h"
#include <vector>

namespace CesiumGeometry {
    class Rectangle;
}

namespace Cesium3DTiles {

    class Tile;

    class CESIUM3DTILES_API TileContent {
    public:
        TileContent();
        virtual ~TileContent();

        virtual const std::string& getType() const = 0;
        
        /**
         * Gives this content a chance to modify its tile. This is the final step of
         * the tile load process, after which the tile state moves from the
         * \ref LoadState::RendererResourcesPrepared state to the
         * \ref LoadState::Done state.
         */
        virtual void finalizeLoad(Tile& tile) = 0;

        virtual void createRasterOverlayTextureCoordinates(
            uint32_t textureCoordinateID,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::Rectangle& rectangle
        ) = 0;
    };

}
