#include "TilesetJson.h"
#include <rapidjson/document.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

	std::optional<BoundingVolume> TilesetJson::getBoundingVolumeProperty(const rapidjson::Value& tileJson, const std::string& key) {
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

	std::optional<double> TilesetJson::getScalarProperty(const rapidjson::Value& tileJson, const std::string& key) {
		auto it = tileJson.FindMember(key.c_str());
		if (it == tileJson.MemberEnd() || !it->value.IsNumber()) {
			return std::nullopt;
		}

		return it->value.GetDouble();
	}

	std::optional<glm::dmat4x4> TilesetJson::getTransformProperty(const rapidjson::Value& tileJson, const std::string& key) {
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

	std::vector<std::string> TilesetJson::getStrings(const rapidjson::Value& json, const std::string& key) {
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
