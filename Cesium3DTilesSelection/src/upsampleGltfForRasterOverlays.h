#pragma once

#include "Cesium3DTilesSelection/Gltf.h"
#include "CesiumGeometry/QuadtreeTileID.h"

namespace Cesium3DTilesSelection {

CesiumGltf::Model upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID);

}
