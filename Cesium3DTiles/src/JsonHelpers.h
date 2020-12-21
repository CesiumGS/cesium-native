#pragma once

#include <optional>
#include "Cesium3DTiles/BoundingVolume.h"
#include <rapidjson/fwd.h>
#include <string>
#include <vector>

namespace Cesium3DTiles {

    class JsonHelpers final {
    public:
        static std::optional<BoundingVolume> getBoundingVolumeProperty(const rapidjson::Value& tileJson, const std::string& key);
        static std::optional<double> getScalarProperty(const rapidjson::Value& tileJson, const std::string& key);
        static std::optional<glm::dmat4x4> getTransformProperty(const rapidjson::Value& tileJson, const std::string& key);
        
        static std::string getStringOrDefault(const rapidjson::Value& json, const std::string& key, const std::string& defaultValue);
        static std::string getStringOrDefault(const rapidjson::Value& json, const std::string& defaultValue);

        static double getDoubleOrDefault(const rapidjson::Value& json, const std::string& key, double defaultValue);
        static double getDoubleOrDefault(const rapidjson::Value& json, double defaultValue);

        static uint32_t getUint32OrDefault(const rapidjson::Value& json, const std::string& key, uint32_t defaultValue);
        static uint32_t getUint32OrDefault(const rapidjson::Value& json, uint32_t defaultValue);

        static std::vector<std::string> getStrings(const rapidjson::Value& json, const std::string& key);
    };

}
