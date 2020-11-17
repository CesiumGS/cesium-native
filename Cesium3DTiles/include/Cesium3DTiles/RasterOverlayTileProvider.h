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
         * @param owner The raster overlay that owns this tile provider.
         * @param tilesetExternals Tileset externals.
         */
        RasterOverlayTileProvider(
            RasterOverlay& owner,
            const TilesetExternals& tilesetExternals
        );

        /**
         * @brief Creates a new instance.
         *
         * @param owner The {@link RasterOverlay}. May not be `nullptr`.
         * @param tilesetExternals The {@link TilesetExternals}.
         * @param projection The {@link CesiumGeospatial::Projection}.
         * @param tilingScheme The {@link CesiumGeometry::QuadtreeTilingScheme}.
         * @param coverageRectangle The {@link CesiumGeometry::Rectangle}.
         * @param minimumLevel The minimum tile level.
         * @param maximumLevel The maximum tile level.
         * @param imageWidth The image width.
         * @param imageHeight The image height.
         */
        RasterOverlayTileProvider(
            RasterOverlay& owner,
            const TilesetExternals& tilesetExternals,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t imageWidth,
            uint32_t imageHeight
        );

        /** @brief Default destructor. */
        virtual ~RasterOverlayTileProvider() {}

        /**
         * @brief Returns whether this is a placeholder. Like this comment.
         */
        bool isPlaceholder() const { return this->_pPlaceholder != nullptr; }

        /**
         * @brief Returns the {@link RasterOverlay} that created this instance.
         */
        RasterOverlay& getOwner() { return *this->_pOwner; }

        /** @copydoc getOwner */
        const RasterOverlay& getOwner() const { return *this->_pOwner; }

        /**
         * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
         */
        const CesiumGeospatial::Projection& getProjection() const { return this->_projection; }

        /**
         * @brief Returns the {@link CesiumGeometry::QuadtreeTilingScheme} of this instance.
         */
        const CesiumGeometry::QuadtreeTilingScheme& getTilingScheme() const { return this->_tilingScheme; }

        /**
         * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this instance.
         */
        const CesiumGeometry::Rectangle& getCoverageRectangle() const { return this->_coverageRectangle; }

        /**
         * @brief Returns the minimum tile level of this instance.
         */
        uint32_t getMinimumLevel() const { return this->_minimumLevel; }

        /**
         * @brief Returns the maximum tile level of this instance.
         */
        uint32_t getMaximumLevel() const { return this->_maximumLevel; }

        /**
         * @brief Returns the image width of this instance.
         */
        uint32_t getWidth() const { return this->_imageWidth; }

        /**
         * @brief Returns the image height of this instance.
         */
        uint32_t getHeight() const { return this->_imageHeight; }

        /**
         * @brief Returns the {@link RasterOverlayTile} with the given ID, requesting it if necessary.
         *
         * @param id The {@link CesiumGeometry::QuadtreeTileID} of the tile to obtain.
         * @return The tile.
         */
        std::shared_ptr<RasterOverlayTile> getTile(const CesiumGeometry::QuadtreeTileID& id);

        /**
         * @brief Returns the {@link RasterOverlayTile} with the given ID, or `nullptr` if there is no such tile.
         *
         * @param id The {@link CesiumGeometry::QuadtreeTileID} of the tile to obtain.
         * @return The tile, or `nullptr`.
         */
        std::shared_ptr<RasterOverlayTile> getTileWithoutRequesting(const CesiumGeometry::QuadtreeTileID& id);

        /**
         * @brief Returns the number of tiles that are currently loading.
         */
        uint32_t getNumberOfTilesLoading() const noexcept;

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

        /**
         * @brief Returns the {@link TilesetExternals} of this instance.
         */
        const TilesetExternals& getExternals() { return *this->_pTilesetExternals; }

        /**
         * @brief Map raster tiles to geometry tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param geometryRectangle The rectangle.
         * @param targetGeometricError The geometric error.
         * @param outputRasterTiles The raster tiles.
         * @param outputIndex The output index.
         */
        void mapRasterTilesToGeometryTile(
            const CesiumGeospatial::GlobeRectangle& geometryRectangle,
            double targetGeometricError,
            std::vector<RasterMappedTo3DTile>& outputRasterTiles,
            std::optional<size_t> outputIndex = std::nullopt
        );

        /** @copydoc mapRasterTilesToGeometryTile */
        void mapRasterTilesToGeometryTile(
            const CesiumGeometry::Rectangle& geometryRectangle,
            double targetGeometricError,
            std::vector<RasterMappedTo3DTile>& outputRasterTiles,
            std::optional<size_t> outputIndex = std::nullopt
        );

    protected:

        /**
         * @brief Returns a new {@link RasterOverlayTile}.
         *
         * Concrete implementations of this class will create the raster overlay tile
         * for the given ID, usually by issuing a network request to a URL that is
         * created by generating a relative URL from the given tile ID and resolving
         * it against a base URL.
         *
         * @param tileID The {@link CesiumGeometry::QuadtreeTileID}
         * @return The new raster overlay tile
         */
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
