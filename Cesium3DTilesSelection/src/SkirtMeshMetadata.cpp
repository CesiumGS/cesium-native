#include "SkirtMeshMetadata.h"

#include <CesiumUtility/JsonValue.h>

#include <optional>
#include <stdexcept>

using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
std::optional<SkirtMeshMetadata>
SkirtMeshMetadata::parseFromGltfExtras(const JsonValue::Object& extras) {
  auto skirtIt = extras.find("skirtMeshMetadata");
  if (skirtIt == extras.end()) {
    return std::nullopt;
  }

  SkirtMeshMetadata skirtMeshMetadata;

  const JsonValue& gltfSkirtMeshMetadata = skirtIt->second;
  const auto* pNoSkirtRange =
      gltfSkirtMeshMetadata.getValuePtrForKey<JsonValue::Array>("noSkirtRange");
  if (!pNoSkirtRange || pNoSkirtRange->size() != 4) {
    return std::nullopt;
  }

  if (!(*pNoSkirtRange)[0].isNumber() || !(*pNoSkirtRange)[1].isNumber() ||
      !(*pNoSkirtRange)[2].isNumber() || !(*pNoSkirtRange)[3].isNumber()) {
    return std::nullopt;
  }

  const double noSkirtIndicesBegin =
      (*pNoSkirtRange)[0].getSafeNumberOrDefault<double>(-1.0);
  const double noSkirtIndicesCount =
      (*pNoSkirtRange)[1].getSafeNumberOrDefault<double>(-1.0);
  const double noSkirtVerticesBegin =
      (*pNoSkirtRange)[2].getSafeNumberOrDefault<double>(-1.0);
  const double noSkirtVerticesCount =
      (*pNoSkirtRange)[3].getSafeNumberOrDefault<double>(-1.0);

  if (noSkirtIndicesBegin < 0.0 || noSkirtIndicesCount < 0.0 ||
      noSkirtVerticesBegin < 0.0 || noSkirtVerticesCount < 0.0) {
    return std::nullopt;
  }

  skirtMeshMetadata.noSkirtIndicesBegin =
      static_cast<uint32_t>(noSkirtIndicesBegin);
  skirtMeshMetadata.noSkirtIndicesCount =
      static_cast<uint32_t>(noSkirtIndicesCount);
  skirtMeshMetadata.noSkirtVerticesBegin =
      static_cast<uint32_t>(noSkirtVerticesBegin);
  skirtMeshMetadata.noSkirtVerticesCount =
      static_cast<uint32_t>(noSkirtVerticesCount);

  const auto* pMeshCenter =
      gltfSkirtMeshMetadata.getValuePtrForKey<JsonValue::Array>("meshCenter");
  if (!pMeshCenter || pMeshCenter->size() != 3) {
    return std::nullopt;
  }

  if (!(*pMeshCenter)[0].isNumber() || !(*pMeshCenter)[1].isNumber() ||
      !(*pMeshCenter)[2].isNumber()) {
    return std::nullopt;
  }

  skirtMeshMetadata.meshCenter = glm::dvec3(
      (*pMeshCenter)[0].getSafeNumberOrDefault<double>(0.0),
      (*pMeshCenter)[1].getSafeNumberOrDefault<double>(0.0),
      (*pMeshCenter)[2].getSafeNumberOrDefault<double>(0.0));

  double westHeight, southHeight, eastHeight, northHeight;
  try {
    westHeight = gltfSkirtMeshMetadata.getSafeNumericalValueForKey<double>(
        "skirtWestHeight");
    southHeight = gltfSkirtMeshMetadata.getSafeNumericalValueForKey<double>(
        "skirtSouthHeight");
    eastHeight = gltfSkirtMeshMetadata.getSafeNumericalValueForKey<double>(
        "skirtEastHeight");
    northHeight = gltfSkirtMeshMetadata.getSafeNumericalValueForKey<double>(
        "skirtNorthHeight");
  } catch (const JsonValueMissingKey&) {
    return std::nullopt;
  } catch (const JsonValueNotRealValue&) {
    return std::nullopt;
  }

  skirtMeshMetadata.skirtWestHeight = westHeight;
  skirtMeshMetadata.skirtSouthHeight = southHeight;
  skirtMeshMetadata.skirtEastHeight = eastHeight;
  skirtMeshMetadata.skirtNorthHeight = northHeight;

  return skirtMeshMetadata;
}

JsonValue::Object SkirtMeshMetadata::createGltfExtras(
    const SkirtMeshMetadata& skirtMeshMetadata) {
  return {
      {"skirtMeshMetadata",
       JsonValue::Object{
           {"noSkirtRange",
            JsonValue::Array{
                skirtMeshMetadata.noSkirtIndicesBegin,
                skirtMeshMetadata.noSkirtIndicesCount,
                skirtMeshMetadata.noSkirtVerticesBegin,
                skirtMeshMetadata.noSkirtVerticesCount}},
           {"meshCenter",
            JsonValue::Array{
                skirtMeshMetadata.meshCenter.x,
                skirtMeshMetadata.meshCenter.y,
                skirtMeshMetadata.meshCenter.z}},
           {"skirtWestHeight", skirtMeshMetadata.skirtWestHeight},
           {"skirtSouthHeight", skirtMeshMetadata.skirtSouthHeight},
           {"skirtEastHeight", skirtMeshMetadata.skirtEastHeight},
           {"skirtNorthHeight", skirtMeshMetadata.skirtNorthHeight}}}};
}
} // namespace Cesium3DTilesSelection
