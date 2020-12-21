#pragma once

#include <optional>
#include "Cesium3DTiles/BoundingVolume.h"
#include <rapidjson/fwd.h>
#include <string>
#include <vector>

namespace Cesium3DTiles {

    class TilesetJson final {
    public:
        static std::optional<BoundingVolume> getBoundingVolumeProperty(const rapidjson::Value& tileJson, const std::string& key);
        static std::optional<double> getScalarProperty(const rapidjson::Value& tileJson, const std::string& key);
        static std::optional<glm::dmat4x4> getTransformProperty(const rapidjson::Value& tileJson, const std::string& key);
        
        static std::vector<std::string> getStrings(const rapidjson::Value& json, const std::string& key);
    };

}
