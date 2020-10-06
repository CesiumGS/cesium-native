#pragma once

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
    };

    class TileContext {
    public:
        Tileset* pTileset;
        std::string baseUrl;
        std::vector<std::pair<std::string, std::string>> requestHeaders;
        std::optional<std::string> version;
        std::optional<ImplicitTilingContext> implicitContext;
    };

}
