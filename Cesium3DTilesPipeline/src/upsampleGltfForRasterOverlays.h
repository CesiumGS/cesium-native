#pragma once

#include "Cesium3DTilesPipeline/Gltf.h"
#include "CesiumGeometry/QuadtreeTileID.h"

namespace Cesium3DTilesPipeline {

CesiumGltf::Model upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID);

}
