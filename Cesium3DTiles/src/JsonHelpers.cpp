// Copyright CesiumGS, Inc. and Contributors

#include "JsonHelpers.h"
#include <rapidjson/document.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

	std::optional<BoundingVolume> JsonHelpers::getBoundingVolumeProperty(const rapidjson::Value& tileJson, const std::string& key) {
		auto bvIt = tileJson.FindMember(key.c_str());
		if (bvIt == tileJson.MemberEnd() || !bvIt->value.IsObject()) {
			return std::nullopt;
		}

		auto boxIt = bvIt->value.FindMember("box");
		if (boxIt != bvIt->value.MemberEnd() && boxIt->value.IsArray() && boxIt->value.Size() >= 12) {
			const auto& a = boxIt->value.GetArray();
			for (rapidjson::SizeType i = 0; i < 12; ++i) {
				if (!a[i].IsNumber()) {
					return std::nullopt;
				}
			}
			return OrientedBoundingBox(
				glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
				glm::dmat3(
					a[3].GetDouble(), a[4].GetDouble(), a[5].GetDouble(),
					a[6].GetDouble(), a[7].GetDouble(), a[8].GetDouble(),
					a[9].GetDouble(), a[10].GetDouble(), a[11].GetDouble()
				)
			);
		}

		auto regionIt = bvIt->value.FindMember("region");
		if (regionIt != bvIt->value.MemberEnd() && regionIt->value.IsArray() && regionIt->value.Size() >= 6) {
			const auto& a = regionIt->value;
			for (rapidjson::SizeType i = 0; i < 6; ++i) {
				if (!a[i].IsNumber()) {
					return std::nullopt;
				}
			}
			return BoundingRegion(GlobeRectangle(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble(), a[3].GetDouble()), a[4].GetDouble(), a[5].GetDouble());
		}

		auto sphereIt = bvIt->value.FindMember("sphere");
		if (sphereIt != bvIt->value.MemberEnd() && sphereIt->value.IsArray() && sphereIt->value.Size() >= 4) {
			const auto& a = sphereIt->value;
			for (rapidjson::SizeType i = 0; i < 4; ++i) {
				if (!a[i].IsNumber()) {
					return std::nullopt;
				}
			}
			return BoundingSphere(glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()), a[3].GetDouble());
		}

		return std::nullopt;
	}

	std::optional<double> JsonHelpers::getScalarProperty(const rapidjson::Value& tileJson, const std::string& key) {
		auto it = tileJson.FindMember(key.c_str());
		if (it == tileJson.MemberEnd() || !it->value.IsNumber()) {
			return std::nullopt;
		}

		return it->value.GetDouble();
	}

	std::optional<glm::dmat4x4> JsonHelpers::getTransformProperty(const rapidjson::Value& tileJson, const std::string& key) {
		auto it = tileJson.FindMember(key.c_str());
		if (it == tileJson.MemberEnd() || !it->value.IsArray() || it->value.Size() < 16) {
			return std::nullopt;
		}

		const auto& a = it->value.GetArray();

		for (rapidjson::SizeType i = 0; i < 16; ++i) {
			if (!a[i].IsNumber()) {
				return std::nullopt;
			}
		}

		return glm::dmat4(
			glm::dvec4(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble(), a[3].GetDouble()),
			glm::dvec4(a[4].GetDouble(), a[5].GetDouble(), a[6].GetDouble(), a[7].GetDouble()),
			glm::dvec4(a[8].GetDouble(), a[9].GetDouble(), a[10].GetDouble(), a[11].GetDouble()),
			glm::dvec4(a[12].GetDouble(), a[13].GetDouble(), a[14].GetDouble(), a[15].GetDouble())
		);
	}

	std::optional<std::vector<double>> JsonHelpers::getDoubles(const rapidjson::Value& json, int32_t expectedSize, const std::string& key) {
		auto it = json.FindMember(key.c_str());
		if (it == json.MemberEnd()) {
			return std::nullopt;
		}
		const auto& value = it->value;
		if (!value.IsArray()) {
			return std::nullopt;
		}
		if (expectedSize >= 0 && value.Size() != static_cast<rapidjson::SizeType>(expectedSize)) {
			return std::nullopt;
		}
		const auto& a = value.GetArray();
		std::vector<double> result;
		result.reserve(value.Size());
		for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
			if (!a[i].IsNumber()) {
				return std::nullopt;
			}
			result.emplace_back(a[i].GetDouble());
		}
		return result;
	}

	std::string JsonHelpers::getStringOrDefault(const rapidjson::Value& json, const std::string& key, const std::string& defaultValue) {
		auto it = json.FindMember(key.c_str());
		return it == json.MemberEnd() ? defaultValue : JsonHelpers::getStringOrDefault(it->value, defaultValue);
	}

	std::string JsonHelpers::getStringOrDefault(const rapidjson::Value& json, const std::string& defaultValue) {
		if (json.IsString()) {
			return json.GetString();
		} else {
			return defaultValue;
		}
	}

	double JsonHelpers::getDoubleOrDefault(const rapidjson::Value& json, const std::string& key, double defaultValue) {
		auto it = json.FindMember(key.c_str());
		return it == json.MemberEnd() ? defaultValue : JsonHelpers::getDoubleOrDefault(it->value, defaultValue);
	}

	double JsonHelpers::getDoubleOrDefault(const rapidjson::Value& json, double defaultValue) {
		if (json.IsDouble()) {
			return json.GetDouble();
		} else {
			return defaultValue;
		}
	}

	uint32_t JsonHelpers::getUint32OrDefault(const rapidjson::Value& json, const std::string& key, uint32_t defaultValue) {
		auto it = json.FindMember(key.c_str());
		return it == json.MemberEnd() ? defaultValue : JsonHelpers::getUint32OrDefault(it->value, defaultValue);
	}

	uint32_t JsonHelpers::getUint32OrDefault(const rapidjson::Value& json, uint32_t defaultValue) {
		if (json.IsUint()) {
			return json.GetUint();
		} else {
			return defaultValue;
		}
	}

	std::vector<std::string> JsonHelpers::getStrings(const rapidjson::Value& json, const std::string& key) {
		std::vector<std::string> result;

        auto it = json.FindMember(key.c_str());
        if (it != json.MemberEnd() && it->value.IsArray()) {
            const auto& valueJson = it->value;
            
			result.reserve(valueJson.Size());

			for (rapidjson::SizeType i = 0; i < valueJson.Size(); ++i) {
				const auto& element = valueJson[i];
                if (element.IsString()) {
					result.emplace_back(element.GetString());
                }
			}
        }

		return result;
	}

}
