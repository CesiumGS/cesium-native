#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Projection.h"
#include <unordered_map>

namespace Cesium3DTiles {

    class TilesetExternals;
    class RasterOverlay;
    class RasterOverlayTile;

    /**
     * @brief Provides individual tiles for a {@link RasterOverlay} on demand.
     * 
     */
    class CESIUM3DTILES_API RasterOverlayTileProvider {
    public:
        /**
         * Constructs a placeholder tile provider.
         * 
         * @param pOverlay The raster overlay.
         * @param tilesetExternals Tileset externals.
         */
        RasterOverlayTileProvider(
            RasterOverlay* pOverlay,
            TilesetExternals& tilesetExternals
        );

        RasterOverlayTileProvider(
            RasterOverlay* pOverlay,
            TilesetExternals& tilesetExternals,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t imageWidth,
            uint32_t imageHeight
        );
        virtual ~RasterOverlayTileProvider() {}

        RasterOverlay* getOverlay() { return this->_pOverlay; }
        const RasterOverlay* getOverlay() const { return this->_pOverlay; }

        const CesiumGeospatial::Projection& getProjection() const { return this->_projection; }
        const CesiumGeometry::QuadtreeTilingScheme& getTilingScheme() const { return this->_tilingScheme; }
        const CesiumGeometry::Rectangle& getCoverageRectangle() const { return this->_coverageRectangle; }
        uint32_t getMinimumLevel() const { return this->_minimumLevel; }
        uint32_t getMaximumLevel() const { return this->_maximumLevel; }
        uint32_t getWidth() const { return this->_imageWidth; }
        uint32_t getHeight() const { return this->_imageHeight; }

        std::shared_ptr<RasterOverlayTile> getTile(const CesiumGeometry::QuadtreeTileID& id, RasterOverlayTileProvider* pOwner = nullptr);
        std::shared_ptr<RasterOverlayTile> getTileWithoutRequesting(const CesiumGeometry::QuadtreeTileID& id);

        /**
         * Computes the appropriate tile level of detail (zoom level) for a given geometric error near
         * a given projected position. The position is required because coordinates in many projections will
         * map to real-world meters differently in different parts of the globe.
         * 
         * @param geometricError The geometric error for which to compute a level.
         * @param position The projected position defining the area of interest.
         * @return The level that is closest to the desired geometric error.
         */
        uint32_t computeLevelFromGeometricError(double geometricError, const glm::dvec2& position) const;

        TilesetExternals& getExternals() { return *this->_pTilesetExternals; }

        void mapRasterTilesToGeometryTile(
            const CesiumGeospatial::GlobeRectangle& geometryRectangle,
            double targetGeometricError,
            std::vector<RasterMappedTo3DTile>& outputRasterTiles,
            std::optional<size_t> outputIndex = std::nullopt
        );

        void mapRasterTilesToGeometryTile(
            const CesiumGeometry::Rectangle& geometryRectangle,
            double targetGeometricError,
            std::vector<RasterMappedTo3DTile>& outputRasterTiles,
            std::optional<size_t> outputIndex = std::nullopt
        );

    protected:
        virtual std::shared_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID, RasterOverlayTileProvider* pOwner = nullptr);

    private:
        RasterOverlay* _pOverlay;
        TilesetExternals* _pTilesetExternals;
        CesiumGeospatial::Projection _projection;
        CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
        CesiumGeometry::Rectangle _coverageRectangle;
        uint32_t _minimumLevel;
        uint32_t _maximumLevel;
        uint32_t _imageWidth;
        uint32_t _imageHeight;
        std::unordered_map<CesiumGeometry::QuadtreeTileID, std::weak_ptr<RasterOverlayTile>> _tiles;
        std::shared_ptr<RasterOverlayTile> _pPlaceholder;
    };
}
