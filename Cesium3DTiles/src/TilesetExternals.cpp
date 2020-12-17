#include "Cesium3DTiles/TilesetExternals.h"
#include "spdlog.h"

namespace Cesium3DTiles {

TilesetExternals::TilesetExternals() noexcept :
    pAssetAccessor(nullptr),
    pPrepareRendererResources(nullptr),
    pTaskProcessor(nullptr),
    pLogger(spdlog::default_logger())
{
}

}
