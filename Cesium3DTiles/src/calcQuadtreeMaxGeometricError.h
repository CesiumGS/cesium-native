#pragma once

#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Ellipsoid.h"

namespace Cesium3DTiles {
double calcQuadtreeMaxGeometricError(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeometry::QuadtreeTilingScheme& tilingScheme);
}