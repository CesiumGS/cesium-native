#pragma once

#include "CesiumGeometry/QuadtreeTileAvailability.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Projection.h"
#include <string>
#include <vector>

namespace Cesium3DTiles {

    class Tileset;

    class ImplicitTilingContext {
    public:
        std::vector<std::string> tileTemplateUrls;
        CesiumGeometry::QuadtreeTilingScheme tilingScheme;
        CesiumGeospatial::Projection projection;
        CesiumGeometry::QuadtreeTileAvailability availability;
    };

    /**
     * @brief The action to take for a failed tile.
     */
    enum class FailedTileAction {
        /**
         * @brief This failure is considered permanent and this tile should not be retried.
         */
        GiveUp,

        /**
         * @brief This tile should be retried immediately.
         */
        Retry,

        /**
         * @brief This tile should be considered failed for now but possibly retried later.
         */
        Wait
    };

    typedef FailedTileAction FailedTileSignature(Tile& failedTile);
    typedef std::function<FailedTileSignature> FailedTileCallback;

    class TileContext {
    public:
        Tileset* pTileset;
        std::string baseUrl;
        std::vector<std::pair<std::string, std::string>> requestHeaders;
        std::optional<std::string> version;
        std::optional<ImplicitTilingContext> implicitContext;
        FailedTileCallback failedTileCallback;
    };

}
