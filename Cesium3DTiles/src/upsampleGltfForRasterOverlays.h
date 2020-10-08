#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "CesiumGeometry/QuadtreeTileID.h"

namespace Cesium3DTiles {

    tinygltf::Model upsampleGltfForRasterOverlays(const tinygltf::Model& parentModel, CesiumGeometry::QuadtreeChild childID);

}
