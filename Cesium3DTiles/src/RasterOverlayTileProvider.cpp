#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/RasterOverlayTile.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

    RasterOverlayTileProvider::RasterOverlayTileProvider(
        TilesetExternals& tilesetExternals,
        const CesiumGeospatial::Projection& projection,
        const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
        uint32_t minimumLevel,
        uint32_t maximumLevel,
        uint32_t imageWidth,
        uint32_t imageHeight
    ) :
        _pTilesetExternals(&tilesetExternals),
        _projection(projection),
        _tilingScheme(tilingScheme),
        _minimumLevel(minimumLevel),
        _maximumLevel(maximumLevel),
        _imageWidth(imageWidth),
        _imageHeight(imageHeight)
    {
    }

    std::shared_ptr<RasterOverlayTile> RasterOverlayTileProvider::getTile(const CesiumGeometry::QuadtreeTileID& id) {
        auto it = this->_tiles.find(id);
        if (it != this->_tiles.end()) {
            std::shared_ptr<RasterOverlayTile> pTile = it->second.lock();
            if (pTile) {
                return pTile;
            }
        }

        std::shared_ptr<RasterOverlayTile> pNew = this->requestNewTile(id);
        this->_tiles[id] = pNew;

        return pNew;
    }

    uint32_t RasterOverlayTileProvider::getLevelWithMaximumTexelSpacing(
        double texelSpacingMeters,
        double latitudeClosestToEquator
    ) const {
        // PERFORMANCE_IDEA: factor out the stuff that doesn't change.
        // TODO: this cimputation is correct only when tilingSchemeRectangle is in meters at the equator, as it is with a Web Mercator projection.
        const QuadtreeTilingScheme& tilingScheme = this->getTilingScheme();
        double latitudeFactor =
            std::get_if<WebMercatorProjection>(&this->getProjection()) != nullptr
                ? std::cos(latitudeClosestToEquator)
                : 1.0;
        const CesiumGeometry::Rectangle& tilingSchemeRectangle = tilingScheme.getRectangle();
        double levelZeroMaximumTexelSpacing =
            (tilingSchemeRectangle.computeWidth() * latitudeFactor) /
            (this->_imageWidth * tilingScheme.getNumberOfXTilesAtLevel(0));

        double twoToTheLevelPower = levelZeroMaximumTexelSpacing / texelSpacingMeters;
        double level = std::log(twoToTheLevelPower) / std::log(2);
        double rounded = std::round(level);
        return static_cast<uint32_t>(rounded);

    }

    void RasterOverlayTileProvider::notifyTileLoaded(RasterOverlayTile& /*tile*/) {
        
    }

}
