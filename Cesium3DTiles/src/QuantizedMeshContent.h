#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include <cstddef>

namespace Cesium3DTiles {

    /**
     * @brief Creates a {@link TileContentLoadResult} from `quantized-mesh-1.0` data.
     */
    class CESIUM3DTILES_API QuantizedMeshContent final : public TileContentLoader {
    public:
        static std::string CONTENT_TYPE;

        /** 
         * @copydoc TileContentLoader::load 
         * 
         * The result will only contain the `model`, `availableTileRectangles`
         * and `updatedBoundingVolume`. Other fields will be empty or have 
         * default values.
         */
        std::unique_ptr<TileContentLoadResult> load(
            const TileContentLoadInput& input) override;

        /**
         * @brief Create a {@link TileContentLoadResult} from the given data. 
         * 
         * (Only public for tests)
         * 
         * @param pLogger Only used for logging
         * @param context The tile context
         * @param tileID The tile ID
         * @param tileBoundingVoume The tile bounding volume
         * @param url The URL
         * @param data The actual input data
         * @return The {@link TileContentLoadResult}
         */
        static std::unique_ptr<TileContentLoadResult> load(
            std::shared_ptr<spdlog::logger> pLogger,
            const TileContext& context,
            const TileID& tileID,
            const BoundingVolume& tileBoundingVolume,
            const std::string& url,
            const gsl::span<const std::byte>& data
        );
    };

}
