#include "calcQuadtreeMaxGeometricError.h"

namespace Cesium3DTiles {
	double calcQuadtreeMaxGeometricError(const CesiumGeospatial::Ellipsoid& ellipsoid, const CesiumGeometry::QuadtreeTilingScheme& tilingScheme)
	{
        static const double mapQuality = 0.25;
        static const uint32_t mapWidth = 65;
        return ellipsoid.getMaximumRadius() * CesiumUtility::Math::TWO_PI * mapQuality
            / (mapWidth * tilingScheme.getRootTilesX());
	}
}