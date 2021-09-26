#pragma once

#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGltf/Model.h"

namespace Cesium3DTilesSelection {

CesiumGltf::Model upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID);

}
