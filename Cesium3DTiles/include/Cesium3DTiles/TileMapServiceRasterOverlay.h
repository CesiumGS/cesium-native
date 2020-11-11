#pragma once

#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include <functional>

namespace Cesium3DTiles {

    /**
     * @brief Options for time map service accesses.
     */
    struct TileMapServiceRasterOverlayOptions {
        //! @cond Doxygen_Suppress
        std::optional<std::string> fileExtension;
        std::optional<std::string> credit;
        std::optional<uint32_t> minimumLevel;
        std::optional<uint32_t> maximumLevel;
        std::optional<CesiumGeometry::Rectangle> coverageRectangle;
        std::optional<CesiumGeospatial::Projection> projection;
        std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;
        std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;
        std::optional<uint32_t> tileWidth;
        std::optional<uint32_t> tileHeight;
        std::optional<bool> flipXY;
        //! @endcond
    };

    /**
     * @brief A {@link RasterOverlay} based on tile map service imagery.
     */
    class CESIUM3DTILES_API TileMapServiceRasterOverlay : public RasterOverlay {
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
            const std::vector<IAssetAccessor::THeader>& headers = std::vector<IAssetAccessor::THeader>(),
            const TileMapServiceRasterOverlayOptions& options = TileMapServiceRasterOverlayOptions()
        );
        virtual ~TileMapServiceRasterOverlay() override;

        virtual void createTileProvider(TilesetExternals& tilesetExternals, std::function<CreateTileProviderCallback>&& callback) override;

    private:
        std::string _url;
        std::vector<IAssetAccessor::THeader> _headers;
        TileMapServiceRasterOverlayOptions _options;

        std::unique_ptr<IAssetRequest> _pMetadataRequest;
        std::function<CreateTileProviderCallback> _callback;
    };

}
