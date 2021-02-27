// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <optional>
#include <spdlog/fwd.h>
#include <unordered_map>

namespace Cesium3DTiles {

    class RasterOverlay;
    class RasterOverlayTile;
    class IPrepareRendererResources;

    struct CESIUM3DTILES_API LoadedRasterOverlayImage {
        std::optional<CesiumGltf::ImageCesium> image;
        std::vector<Credit> credits;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

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
         * @param asyncSystem The async system used to do work in threads.
         * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for this raster overlay.
         */
        RasterOverlayTileProvider(
            RasterOverlay& owner,
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor
        ) noexcept;

        /**
         * @brief Creates a new instance.
         *
         * @param owner The {@link RasterOverlay}. May not be `nullptr`.
         * @param asyncSystem The async system used to do work in threads.
         * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for this raster overlay.
         * @param credit The {@link Credit} for this tile provider, if it exists.
         * @param pPrepareRendererResources The interface used to prepare raster images for rendering.
         * @param pLogger The logger to which to send messages about the tile provider and tiles.
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
            const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
            std::optional<Credit> credit,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            std::shared_ptr<spdlog::logger> pLogger,
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
        const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() const noexcept { return this->_pAssetAccessor; }

        /**
         * @brief Gets the async system used to do work in threads.
         */
        const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept { return this->_asyncSystem; }

        /**
         * @brief Gets the interface used to prepare raster overlay images for rendering.
         */
        const std::shared_ptr<IPrepareRendererResources>& getPrepareRendererResources() const noexcept { return this->_pPrepareRendererResources; }

        /**
         * @brief Gets the logger to which to send messages about the tile provider and tiles.
         */
        const std::shared_ptr<spdlog::logger>& getLogger() const noexcept { return this->_pLogger; }

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
         * @brief Returns the {@link RasterOverlayTile} with the given ID, creating it if necessary.
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
        CesiumUtility::IntrusivePointer<RasterOverlayTile> getTileWithoutCreating(const CesiumGeometry::QuadtreeTileID& id);

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
         * @brief Gets the number of bytes of tile data that are currently loaded.
         */
        int64_t getTileDataBytes() const noexcept { return this->_tileDataBytes; }

        /**
         * @brief Returns the number of tiles that are currently loading.
         */
        uint32_t getNumberOfTilesLoading() const noexcept { return this->_totalTilesCurrentlyLoading; }

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
        const std::optional<Credit> getCredit() const noexcept { return _credit; }

        /**
         * @brief Loads a tile immediately, without throttling requests.
         * 
         * If the tile is not in the `Tile::LoadState::Unloading` state, this method
         * returns without doing anything. Otherwise, it puts the tile into the
         * `Tile::LoadState::Loading` state and begins the asynchronous process
         * to load the tile. When the process completes, the tile will be in the
         * `Tile::LoadState::Loaded` or `Tile::LoadState::Failed` state.
         * 
         * Calling this method on many tiles at once can result in very slow
         * performance. Consider using {@link loadTileThrottled} instead.
         * 
         * @param tile The tile to load.
         */
        void loadTile(RasterOverlayTile& tile);

        /**
         * @brief Loads a tile, unless there are too many tile loads already in progress.
         * 
         * If the tile is not in the `Tile::LoadState::Unloading` state, this method
         * returns true without doing anything. If too many tile loads are
         * already in flight, it returns false without doing anything. Otherwise, it
         * puts the tile into the `Tile::LoadState::Loading` state, begins the asynchronous process
         * to load the tile, and returns true. When the process completes, the tile will be in the
         * `Tile::LoadState::Loaded` or `Tile::LoadState::Failed` state.
         * 
         * The number of allowable simultaneous tile requests is provided in the
         * {@link RasterOverlayOptions::maximumSimultaneousTileLoads} property of {@link RasterOverlay::getOptions}.
         * 
         * @param tile The tile to load.
         * @returns True if the tile load process is started or is already complete, false if the load
         *          could not be started because too many loads are already in progress.
         */
        bool loadTileThrottled(RasterOverlayTile& tile);

    protected:
        /**
         * @brief Loads the image for a tile.
         * 
         * @param tileID The ID of the tile to load.
         * @return A future that resolves to the image or error information.
         */
        virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const = 0;

        /**
         * @brief Loads an image from a URL and optionally some request headers.
         * 
         * This is a useful helper function for implementing {@link loadTileImage}.
         * 
         * @param url The URL.
         * @param headers The request headers.
         * @return A future that resolves to the image or error information.
         */
        CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImageFromUrl(
            const std::string& url,
            const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
            const std::vector<Credit>& credits = {}
        ) const;

    private:
        void doLoad(RasterOverlayTile& tile, bool isThrottledLoad);

        /**
         * @brief Begins the process of loading of a tile.
         * 
         * This method should be called at the beginning of the tile load process.
         * 
         * @param tile The tile that is starting to load.
         * @param isThrottledLoad True if the load was originally throttled.
         */
        void beginTileLoad(RasterOverlayTile& tile, bool isThrottledLoad);

        /**
         * @brief Finalizes loading of a tile.
         * 
         * This method should be called at the end of the tile load process,
         * no matter whether the load succeeded or failed.
         * 
         * @param tile The tile that finished loading.
         * @param isThrottledLoad True if the load was originally throttled.
         */
        void finalizeTileLoad(RasterOverlayTile& tile, bool isThrottledLoad);

        RasterOverlay* _pOwner;
        CesiumAsync::AsyncSystem _asyncSystem;
        std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
        std::optional<Credit> _credit;
        std::shared_ptr<IPrepareRendererResources> _pPrepareRendererResources;
        std::shared_ptr<spdlog::logger> _pLogger;
        CesiumGeospatial::Projection _projection;
        CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
        CesiumGeometry::Rectangle _coverageRectangle;
        uint32_t _minimumLevel;
        uint32_t _maximumLevel;
        uint32_t _imageWidth;
        uint32_t _imageHeight;
        std::unordered_map<CesiumGeometry::QuadtreeTileID, std::unique_ptr<RasterOverlayTile>> _tiles;
        std::unique_ptr<RasterOverlayTile> _pPlaceholder;
        int64_t _tileDataBytes;
        int32_t _totalTilesCurrentlyLoading;
        int32_t _throttledTilesCurrentlyLoading;
    };
}
