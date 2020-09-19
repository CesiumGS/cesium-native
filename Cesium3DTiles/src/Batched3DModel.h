#pragma once

#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/CompleteTileDefinition.h"
#include <memory>
#include <string>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API Batched3DModel {
    public:
        static std::string MAGIC;
        static std::unique_ptr<GltfContent> load(
            const TileContext& tileContext,
            const CompleteTileDefinition& tile,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        );
    };

}
