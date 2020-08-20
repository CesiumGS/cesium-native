#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/GltfContent.h"
#include <string>
#include <memory>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Batched3DModel {
    public:
        static std::string MAGIC;
        static std::unique_ptr<GltfContent> load(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url);
    };

}
