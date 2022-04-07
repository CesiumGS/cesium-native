#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <Cesium3DTilesSelection/Exp_TileRenderContentResolver.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<RenderContent> TileRenderContentResolver::load(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
    CesiumAsync::AsyncSystem& asyncSystem,
    CesiumAsync::IAssetAccessor& assetAccessor) {}
} // namespace Cesium3DTilesSelection
