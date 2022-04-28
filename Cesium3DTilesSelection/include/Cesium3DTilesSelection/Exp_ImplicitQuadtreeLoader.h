#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
class ImplicitQuadtreeLoader : public TilesetContentLoader {
public:
  ImplicitQuadtreeLoader(std::string&& baseUrl);

  CesiumAsync::Future<TileLoadResult> loadTileContent(
      TilesetContentLoader& currentLoader,
      const TileContentLoadInfo& loadInfo,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

private:
  std::string _baseUrl;
};
} // namespace Cesium3DTilesSelection
