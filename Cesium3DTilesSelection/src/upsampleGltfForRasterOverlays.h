#pragma once

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Model.h>

namespace Cesium3DTilesSelection {

std::optional<CesiumGltf::Model> upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    int32_t textureCoordinateIndex = 0);

}
