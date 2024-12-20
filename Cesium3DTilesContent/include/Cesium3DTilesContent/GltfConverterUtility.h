#pragma once

#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumUtility/ErrorList.h>

#include <glm/fwd.hpp>
#include <rapidjson/document.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace CesiumGltf {
struct Model;
struct Buffer;
} // namespace CesiumGltf

namespace Cesium3DTilesContent {

namespace GltfConverterUtility {
std::optional<uint32_t> parseOffsetForSemantic(
    const rapidjson::Document& document,
    const char* semantic,
    CesiumUtility::ErrorList& errorList);

typedef bool (rapidjson::Value::*ValuePredicate)() const;

template <typename T> bool isValue(const rapidjson::Value& value);
template <typename T> T getValue(const rapidjson::Value& value);

template <typename T>
std::optional<T> getOptional(const rapidjson::Value& value) {
  if (isValue<T>(value)) {
    return std::make_optional(getValue<T>(value));
  }
  return {};
}

template <typename T>
std::optional<T>
getValue(const rapidjson::Document& document, const char* semantic) {
  const auto valueIt = document.FindMember(semantic);
  if (valueIt == document.MemberEnd() || !isValue<T>(valueIt->value)) {
    return {};
  }
  return std::make_optional(getValue<T>(valueIt->value));
}

template <> inline bool isValue<bool>(const rapidjson::Value& value) {
  return value.IsBool();
}

template <> inline bool getValue<bool>(const rapidjson::Value& value) {
  return value.GetBool();
}

template <> inline bool isValue<uint32_t>(const rapidjson::Value& value) {
  return value.IsUint();
}

template <> inline uint32_t getValue<uint32_t>(const rapidjson::Value& value) {
  return value.GetUint();
}

bool validateJsonArrayValues(
    const rapidjson::Value& arrayValue,
    uint32_t expectedLength,
    ValuePredicate predicate);

std::optional<glm::dvec3>
parseArrayValueDVec3(const rapidjson::Value& arrayValue);

std::optional<glm::dvec3>
parseArrayValueDVec3(const rapidjson::Document& document, const char* name);

int32_t
createBufferInGltf(CesiumGltf::Model& gltf, std::vector<std::byte> buffer = {});

int32_t createBufferViewInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferId,
    const int64_t byteLength,
    const int64_t byteStride);

int32_t createAccessorInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferViewId,
    const int32_t componentType,
    const int64_t count,
    const std::string& type);

/**
 * Applies the given relative-to-center (RTC) translation to the transforms of
 * all nodes in the glTF. This is useful in converting i3dm files, where the RTC
 * translation must be applied to the model before the i3dm instance
 * transform. It's also the 3D Tiles 1.1 "way" to do away with RTC and encode it
 * directly in the glTF.
 */
void applyRtcToNodes(CesiumGltf::Model& gltf, const glm::dvec3& rtc);

template <typename GlmType, typename GLTFType>
GlmType toGlm(const GLTFType& gltfVal);

template <typename GlmType, typename ComponentType>
GlmType toGlm(const CesiumGltf::AccessorTypes::VEC3<ComponentType>& gltfVal) {
  return GlmType(gltfVal.value[0], gltfVal.value[1], gltfVal.value[2]);
}

template <typename GlmType, typename ComponentType>
GlmType
toGlmQuat(const CesiumGltf::AccessorTypes::VEC4<ComponentType>& gltfVal) {
  if constexpr (std::is_same<ComponentType, float>()) {
    return GlmType(
        gltfVal.value[3],
        gltfVal.value[0],
        gltfVal.value[1],
        gltfVal.value[2]);
  } else {
    return GlmType(
        CesiumGltf::normalize(gltfVal.value[3]),
        CesiumGltf::normalize(gltfVal.value[0]),
        CesiumGltf::normalize(gltfVal.value[1]),
        CesiumGltf::normalize(gltfVal.value[2]));
  }
}

} // namespace GltfConverterUtility
} // namespace Cesium3DTilesContent
