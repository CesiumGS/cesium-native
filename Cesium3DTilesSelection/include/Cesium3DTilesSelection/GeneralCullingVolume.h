#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Library.h>
#include <CesiumGeometry/CullingVolume.h>

#include <variant>

namespace Cesium3DTilesSelection {

using GeneralCullingVolume =
    std::variant<CesiumGeometry::CullingVolume, BoundingVolume>;

bool CESIUM3DTILESSELECTION_API isBoundingVolumeVisible(const GeneralCullingVolume& cullingVolume, const BoundingVolume& boundingVolume);

} // namespace Cesium3DTilesSelection
