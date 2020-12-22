#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <unordered_map>
#include <optional>

namespace Cesium3DTiles {

    class RasterOverlay;
    class RasterOverlayTile;
    class IPrepareRendererResources;

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
         * @param asyncSystem The async system used to request assets and do work in threads.
         */
        RasterOverlayTileProvider(
            RasterOverlay& owner,
            const CesiumAsync::AsyncSystem& asyncSystem
        ) noexcept;

        /**
         * @brief Creates a new instance.
         *
         * @param owner The {@link RasterOverlay}. May not be `nullptr`.
         * @param asyncSystem The async system used to request assets and do work in threads.
         * @param credit The {@link Credit} for this tile provider, if it exists.
         * @param pPrepareRendererResources The interface used to prepare raster images for rendering.
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
            const CesiumAsync::AsyncSystem& asyncSystem,
            std::optional<Credit> credit,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t imageWidth,
            uint32_t imageHeight
        ) noexcept;

        /** @brief Default destructor. */
        virtual ~RasterOverlayTileProvider() {}

        /**
         * @brief Returns whether this is a placeholder. Like this comment.
         */
        bool isPlaceholder() const noexcept { return this->_pPlaceholder != nullptr; }

        /**
         * @brief Returns the {@link RasterOverlay} that created this instance.
         */
        RasterOverlay& getOwner() noexcept { return *this->_pOwner; }

        /** @copydoc getOwner */
        const RasterOverlay& getOwner() const noexcept { return *this->_pOwner; }

        /**
         * @brief Get the system to use for asychronous requests and threaded work.
         */
        CesiumAsync::AsyncSystem& getAsyncSystem() noexcept { return this->_asyncSystem; }

        /** @copydoc getAsyncSystem */
        const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept { return this->_asyncSystem; }

        /**
         * @brief Gets the interface used to prepare raster overlay images for rendering.
         */
        const std::shared_ptr<IPrepareRendererResources>& getPrepareRendererResources() const noexcept { return this->_pPrepareRendererResources; }

        /**
         * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
         */
        const CesiumGeospatial::Projection& getProjection() const noexcept { return this->_projection; }

        /**
         * @brief Returns the {@link CesiumGeometry::QuadtreeTilingScheme} of this instance.
         */
        const CesiumGeometry::QuadtreeTilingScheme& getTilingScheme() const noexcept { return this->_tilingScheme; }

        /**
         * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this instance.
         */
        const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept { return this->_coverageRectangle; }

        /**
         * @brief Returns the minimum tile level of this instance.
         */
        uint32_t getMinimumLevel() const noexcept { return this->_minimumLevel; }

        /**
         * @brief Returns the maximum tile level of this instance.
         */
        uint32_t getMaximumLevel() const noexcept { return this->_maximumLevel; }

        /**
         * @brief Returns the image width of this instance.
         */
        uint32_t getWidth() const noexcept { return this->_imageWidth; }

        /**
         * @brief Returns the image height of this instance.
         */
        uint32_t getHeight() const noexcept { return this->_imageHeight; }

        /**
         * @brief Returns the {@link RasterOverlayTile} with the given ID, requesting it if necessary.
         *
         * @param id The {@link CesiumGeometry::QuadtreeTileID} of the tile to obtain.
         * @return The tile.
         */
        CesiumUtility::IntrusivePointer<RasterOverlayTile> getTile(const CesiumGeometry::QuadtreeTileID& id);

        /**
         * @brief Returns the {@link RasterOverlayTile} with the given ID, or `nullptr` if there is no such tile.
         *
         * @param id The {@link CesiumGeometry::QuadtreeTileID} of the tile to obtain.
         * @return The tile, or `nullptr`.
         */
        CesiumUtility::IntrusivePointer<RasterOverlayTile> getTileWithoutRequesting(const CesiumGeometry::QuadtreeTileID& id);

        /**
         * Computes the appropriate tile level of detail (zoom level) for a given geometric error near
         * a given projected position. The position is required because coordinates in many projections will
         * map to real-world meters differently in different parts of the globe.
         * 
         * @param geometricError The geometric error for which to compute a level.
         * @param position The projected position defining the area of interest.
         * @return The level that is closest to the desired geometric error.
         */
        uint32_t computeLevelFromGeometricError(double geometricError, const glm::dvec2& position) const noexcept;

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

        /**
         * @brief Notifies the tile provider that a tile has finished loading.
         * 
         * This function is not supposed to be called by clients.
         */
        void notifyTileLoaded(RasterOverlayTile* pTile) noexcept;

        /**
         * @brief Gets the number of bytes of tile data that are currently loaded.
         */
        size_t getTileDataBytes() const noexcept { return this->_tileDataBytes; }

        /**
         * @brief Returns the number of tiles that are currently loading.
         */
        uint32_t getNumberOfTilesLoading() const noexcept { return this->_tilesCurrentlyLoading; }

        /**
         * @brief Removes a no-longer-referenced tile from this provider's cache and deletes it.
         * 
         * This function is not supposed to be called by client. Calling this method in a tile
         * with a reference count greater than 0 will result in undefined behavior.
         * 
         * @param pTile The tile, which must have no oustanding references.
         */
        void removeTile(RasterOverlayTile* pTile) noexcept;

        /**
         * @brief Get the per-TileProvider {@link Credit} if one exists.
         */
        std::optional<Credit> getCredit() const noexcept { return _credit; }

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
        virtual std::unique_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID);

    private:
        RasterOverlay* _pOwner;
        CesiumAsync::AsyncSystem _asyncSystem;
        std::optional<Credit> _credit;
        std::shared_ptr<IPrepareRendererResources> _pPrepareRendererResources;
        CesiumGeospatial::Projection _projection;
        CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
        CesiumGeometry::Rectangle _coverageRectangle;
        uint32_t _minimumLevel;
        uint32_t _maximumLevel;
        uint32_t _imageWidth;
        uint32_t _imageHeight;
        std::unordered_map<CesiumGeometry::QuadtreeTileID, std::unique_ptr<RasterOverlayTile>> _tiles;
        std::unique_ptr<RasterOverlayTile> _pPlaceholder;
        size_t _tileDataBytes;
        uint32_t _tilesCurrentlyLoading;
    };
}
