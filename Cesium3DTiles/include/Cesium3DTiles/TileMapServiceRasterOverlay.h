#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <functional>

namespace Cesium3DTiles {

    /**
     * @brief Options for tile map service accesses.
     */
    struct TileMapServiceRasterOverlayOptions {
        
        /** 
         * @brief The file extension for images on the server. 
         */
        std::optional<std::string> fileExtension;

        /** 
         * @brief A credit for the data source, which is displayed on the canvas. 
         */
        std::optional<std::string> credit;

        /**
         * @brief The minimum level-of-detail supported by the imagery provider. 
         * 
         * Take care when specifying this that the number of tiles at the minimum 
         * level is small, such as four or less. A larger number is likely to 
         * result in rendering problems.
         */
        std::optional<uint32_t> minimumLevel;

        /**
         * @brief The maximum level-of-detail supported by the imagery provider.
         *
         * This will be `std::nullopt` if there is no limit.
         */
        std::optional<uint32_t> maximumLevel;

        /**
         * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the image.
         */
        std::optional<CesiumGeometry::Rectangle> coverageRectangle;

        /**
         * @brief The {@link CesiumGeospatial::Projection} that is used.
         */
        std::optional<CesiumGeospatial::Projection> projection;

        /**
         * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} specifying how 
         * the ellipsoidal surface is broken into tiles. 
         */
        std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;

        /**
         * @brief The {@link CesiumGeospatial::Ellipsoid}. 
         *
         * If the `tilingScheme` is specified, this parameter is ignored and 
         * the tiling scheme's ellipsoid is used instead. If neither parameter 
         * is specified, the {@link CesiumGeospatial::Ellipsoid::WGS84} is used.
         */
        std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;

        /**
         * @brief Pixel width of image tiles.
         */
        std::optional<uint32_t> tileWidth;

        /**
         * @brief Pixel height of image tiles.
         */
        std::optional<uint32_t> tileHeight;

        /**
         * @brief An otion to flip the x- and y values of a tile map resource.
         *
         * Older versions of gdal2tiles.py flipped X and Y values in `tilemapresource.xml`. 
         * Specifying this option will do the same, allowing for loading of these 
         * incorrect tilesets.
         */
        std::optional<bool> flipXY;
    };

    /**
     * @brief A {@link RasterOverlay} based on tile map service imagery.
     */
    class CESIUM3DTILES_API TileMapServiceRasterOverlay final : public RasterOverlay {
    public:

        /**
         * @brief Creates a new instance.
         * 
         * @param url The base URL.
         * @param headers The headers. This is a list of pairs of strings of the
         * form (Key,Value) that will be inserted as request headers internally.
         * @param options The {@link TileMapServiceRasterOverlayOptions}.
         */
        TileMapServiceRasterOverlay(
            const std::string& url,
            const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = std::vector<CesiumAsync::IAssetAccessor::THeader>(),
            const TileMapServiceRasterOverlayOptions& options = TileMapServiceRasterOverlayOptions()
        );
        virtual ~TileMapServiceRasterOverlay() override;

        virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>> createTileProvider(
            const CesiumAsync::AsyncSystem& asyncSystem,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            std::shared_ptr<spdlog::logger> pLogger,
            RasterOverlay* pOwner
        ) override;

    private:
        std::string _url;
        std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
        TileMapServiceRasterOverlayOptions _options;
    };

}
