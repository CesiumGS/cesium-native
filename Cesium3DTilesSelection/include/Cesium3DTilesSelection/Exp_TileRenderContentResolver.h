#pragma once

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <memory>

namespace Cesium3DTilesSelection {
struct TileRenderContentResolver {
  static CesiumAsync::Future<RenderContent> load(
      const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
      CesiumAsync::AsyncSystem& asyncSystem,
      CesiumAsync::IAssetAccessor& assetAccessor);
};
} // namespace Cesium3DTilesSelection
