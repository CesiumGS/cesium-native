#include "Cesium3DTiles/RasterOverlayTileProvider.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

    RasterOverlayTileProvider::RasterOverlayTileProvider(
        const CesiumGeospatial::Projection& projection,
        const CesiumGeospatial::QuadtreeTilingScheme& tilingScheme,
        uint32_t minimumLevel,
        uint32_t maximumLevel,
        uint32_t imageWidth,
        uint32_t imageHeight
    ) :
        _projection(projection),
        _tilingScheme(tilingScheme),
        _minimumLevel(minimumLevel),
        _maximumLevel(maximumLevel),
        _imageWidth(imageWidth),
        _imageHeight(imageHeight)
    {
    }

    uint32_t RasterOverlayTileProvider::getLevelWithMaximumTexelSpacing(
        double totalWidthMeters,
        double texelSpacingMeters,
        double latitudeClosestToEquator
    ) const {
        // PERFORMANCE_IDEA: factor out the stuff that doesn't change.
        const QuadtreeTilingScheme& tilingScheme = this->getTilingScheme();
        double latitudeFactor =
            std::get_if<WebMercatorProjection>(&this->getProjection()) != nullptr
                ? std::cos(latitudeClosestToEquator)
                : 1.0;
        const CesiumGeometry::Rectangle& tilingSchemeRectangle = tilingScheme.getRectangle();
        double levelZeroMaximumTexelSpacing =
            (totalWidthMeters * tilingSchemeRectangle.computeWidth() * latitudeFactor) /
            (this->_imageWidth * tilingScheme.getNumberOfXTilesAtLevel(0));

        double twoToTheLevelPower = levelZeroMaximumTexelSpacing / texelSpacingMeters;
        double level = std::log(twoToTheLevelPower) / std::log(2);
        double rounded = std::round(level);
        return static_cast<uint32_t>(rounded);

    }

}
