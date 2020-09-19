#pragma once

#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Projection.h"
#include <string>
#include <vector>

namespace Cesium3DTiles {

    class Tileset;

    struct ImplicitTilingContext {
        std::vector<std::string> tileTemplateUrls;
        CesiumGeometry::QuadtreeTilingScheme tilingScheme;
        CesiumGeospatial::Projection projection;
    };

    struct TileContext {
        Tileset* pTileset; // TODO: remove this
        std::string baseUrl;
        std::optional<ImplicitTilingContext> implicitContext;
    };

}
