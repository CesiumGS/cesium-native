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
         * @param pOwner The raster overlay that owns this tile provider.
         * @param tilesetExternals Tileset externals.
         */
        RasterOverlayTileProvider(
            RasterOverlay& owner,
            const TilesetExternals& tilesetExternals
        );

        RasterOverlayTileProvider(
            RasterOverlay& wwner,
            const TilesetExternals& tilesetExternals,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t imageWidth,
            uint32_t imageHeight
        );
        virtual ~RasterOverlayTileProvider() {}

        RasterOverlay& getOwner() { return *this->_pOwner; }
        const RasterOverlay& getOwner() const { return *this->_pOwner; }

        const CesiumGeospatial::Projection& getProjection() const { return this->_projection; }
        const CesiumGeometry::QuadtreeTilingScheme& getTilingScheme() const { return this->_tilingScheme; }
        const CesiumGeometry::Rectangle& getCoverageRectangle() const { return this->_coverageRectangle; }
        uint32_t getMinimumLevel() const { return this->_minimumLevel; }
        uint32_t getMaximumLevel() const { return this->_maximumLevel; }
        uint32_t getWidth() const { return this->_imageWidth; }
        uint32_t getHeight() const { return this->_imageHeight; }

        std::shared_ptr<RasterOverlayTile> getTile(const CesiumGeometry::QuadtreeTileID& id);
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

        const TilesetExternals& getExternals() { return *this->_pTilesetExternals; }

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

        /**
         * @brief Safely destroys this overlay tile provider and its associated overlay.
         * 
         * The overlay will not be truly destroyed until all in-progress tile loads complete. This may happen
         * before this function returns if no loads are in progress.
         * 
         * @param pOverlayTileProvider A unique pointer to this instance, allowing it to take ownership of itself.
         * @param pOverlay A unique pointer to the corresponding overlay. Ownership will be taken of this overlay.
         */
        void destroySafely(std::unique_ptr<RasterOverlayTileProvider>&& pOverlayTileProvider, std::unique_ptr<RasterOverlay>&& pOverlay);

    protected:
        virtual std::shared_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID);

    private:
        RasterOverlay* _pOwner;
        const TilesetExternals* _pTilesetExternals;
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
