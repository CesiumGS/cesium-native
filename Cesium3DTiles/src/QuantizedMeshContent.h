#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadResult.h"

namespace Cesium3DTiles {

    /**
     * @brief Creates a {@link TileContentLoadResult} from `quantized-mesh-1.0` data.
     */
    class CESIUM3DTILES_API QuantizedMeshContent final {
    public:
        static std::string CONTENT_TYPE;

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
