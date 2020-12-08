#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include <memory>
#include <string>

namespace Cesium3DTiles {

    class Tileset;

    /**
     * @brief Creates a {@link TileContentLoadResult} from B3DM data.
     */
    class CESIUM3DTILES_API Batched3DModelContent final {
    public:
        /** @copydoc ExternalTilesetContent::load */
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
