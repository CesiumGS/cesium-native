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
enum class TileRenderContentFailReason {
  DataRequestFailed,
  UnsupportedFormat,
  ConversionFailed,
  Success
};

struct TileRenderContentLoadResult {
  TileRenderContent content;
  TileRenderContentFailReason reason;
};

struct TileRenderContentLoader {
  static CesiumAsync::Future<TileRenderContentLoadResult> load(
      const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
      const TileContentLoadInfo& loadInfo);
};
} // namespace Cesium3DTilesSelection
