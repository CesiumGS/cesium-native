// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include <memory>
#include <string>
#include <spdlog/fwd.h>

namespace Cesium3DTiles {

    class Tileset;

    /**
     * @brief Creates a {@link TileContentLoadResult} from CMPT data.
     */
    class CESIUM3DTILES_API CompositeContent final : public TileContentLoader {
    public:
        std::unique_ptr<TileContentLoadResult> load(
            const TileContentLoadInput& input) override;
    };

}
