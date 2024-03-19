#pragma once

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Model.h>

namespace Cesium3DTilesContent {

std::optional<CesiumGltf::Model> upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    const std::string& textureCoordinateAttributeBaseName = "TEXCOORD_",
    int32_t textureCoordinateIndex = 0);

}
