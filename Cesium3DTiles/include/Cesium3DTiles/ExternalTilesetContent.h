#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileRefine.h"
#include <gsl/span>
#include <memory>
#include <vector>

namespace Cesium3DTiles {

    class Tileset;

    /**
     * @brief A class that can create a {@link TileContentLoadResult} from JSON data.
     */
    class CESIUM3DTILES_API ExternalTilesetContent {
    public:

        /**
         * @brief Create the {@link TileContentLoadResult} from the given input data.
         * 
         * @param context The {@link TileContext}
         * @param tileID The {@link TileID}
         * @param tileBoundingVolume The tile {@link BoundingVolume}
         * @param tileGeometricError The geometric error
         * @param tileTransform The tile transform
         * @param tileContentBoundingVolume Tile content {@link BoundingVolume}
         * @param tileRefine The {@link TileRefine}
         * @param url The source URL
         * @param data The raw input data
         * @return The {@link TileContentLoadResult}
         */
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
    };

}
