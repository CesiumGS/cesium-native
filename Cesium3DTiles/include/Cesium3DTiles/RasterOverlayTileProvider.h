#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayTileRequest.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumGeospatial/QuadtreeTilingScheme.h"

namespace Cesium3DTiles {

    class IAssetAccessor;

    /**
     * @brief Provides individuals tiles for a {@link RasterOverlay} on demand.
     * 
     */
    class CESIUM3DTILES_API RasterOverlayTileProvider {
    public:
        RasterOverlayTileProvider(
            const CesiumGeospatial::Projection& projection,
            const CesiumGeospatial::QuadtreeTilingScheme& tilingScheme,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t imageWidth,
            uint32_t imageHeight
        );
        virtual ~RasterOverlayTileProvider() = 0 {}

        const CesiumGeospatial::Projection& getProjection() const { return this->_projection; }
        const CesiumGeospatial::QuadtreeTilingScheme& getTilingScheme() const { return this->_tilingScheme; }
        uint32_t getMinimumLevel() const { return this->_minimumLevel; }
        uint32_t getMaximumLevel() const { return this->_maximumLevel; }
        uint32_t getWidth() const { return this->_imageWidth; }
        uint32_t getHeight() const { return this->_imageHeight; }

        virtual std::unique_ptr<RasterOverlayTileRequest> requestImage(const CesiumGeometry::QuadtreeTileID& id, IAssetAccessor& accessor) = 0;

        uint32_t getLevelWithMaximumTexelSpacing(double totalWidthMeters, double texelSpacingMeters, double latitudeClosestToEquator) const;

    private:
        CesiumGeospatial::Projection _projection;
        CesiumGeospatial::QuadtreeTilingScheme _tilingScheme;
        uint32_t _minimumLevel;
        uint32_t _maximumLevel;
        uint32_t _imageWidth;
        uint32_t _imageHeight;
    };
}
