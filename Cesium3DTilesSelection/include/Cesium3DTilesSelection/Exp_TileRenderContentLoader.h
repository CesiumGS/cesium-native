#pragma once

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <Cesium3DTilesSelection/Exp_TileContentLoadInfo.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <gsl/span>

#include <memory>

namespace Cesium3DTilesSelection {
struct TileRenderContentLoadResult {
  TileRenderContent content;
  TileLoadState state;
};

class TileRenderContentLoader {
public:
  static bool canCreateRenderContent(
      const std::string& tileUrl,
      const gsl::span<const std::byte>& tileContentBinary);

  static CesiumAsync::Future<TileRenderContentLoadResult> load(
      const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
      const TileContentLoadInfo& loadInfo);
};
} // namespace Cesium3DTilesSelection
