#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "CesiumGeometry/QuadtreeTileID.h"

namespace Cesium3DTiles {

CesiumGltf::Model upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID);

}
