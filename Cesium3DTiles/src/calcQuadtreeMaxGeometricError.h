#pragma once

#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"

namespace Cesium3DTiles {
    double calcQuadtreeMaxGeometricError(const CesiumGeospatial::Ellipsoid &ellipsoid, const CesiumGeometry::QuadtreeTilingScheme &tilingScheme);
}