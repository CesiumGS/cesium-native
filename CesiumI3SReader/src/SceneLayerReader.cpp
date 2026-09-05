#include "BuildingLayerJsonHandlers.h"
#include "PointCloudJsonHandlers.h"
#include "SceneLayerJsonHandler.h"

#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumJsonReader/JsonReader.h>

namespace CesiumI3SReader {

SceneLayerReader::SceneLayerReader() noexcept = default;

CesiumJsonReader::ReadJsonResult<CesiumI3S::Layer>
SceneLayerReader::readLayer(const std::span<const std::byte>& data) const {
  LayerJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

CesiumJsonReader::ReadJsonResult<CesiumI3S::BuildingLayer>
SceneLayerReader::readBuildingLayer(
    const std::span<const std::byte>& data) const {
  BuildingLayerJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

CesiumJsonReader::ReadJsonResult<CesiumI3S::PointCloudLayer>
SceneLayerReader::readPointCloudLayer(
    const std::span<const std::byte>& data) const {
  PointCloudLayerJsonHandler handler;
  return CesiumJsonReader::JsonReader::readJson(data, handler);
}

} // namespace CesiumI3SReader
