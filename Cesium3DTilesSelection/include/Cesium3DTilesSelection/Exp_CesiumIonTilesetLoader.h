#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
#include <Cesium3DTilesSelection/CreditSystem.h>

#include <string>

namespace Cesium3DTilesSelection {
class CesiumIonTilesetLoader : public TilesetContentLoader {
  enum class TokenRefreshState { None, Loading, Done, Failed };

public:
  CesiumIonTilesetLoader(
      const TilesetExternals& externals,
      uint32_t ionAssetID,
      std::string&& ionAccessToken,
      std::string&& ionAssetEndpointUrl,
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader);

  CesiumAsync::Future<TileLoadResult> doLoadTileContent(
      Tile& tile,
      const TilesetContentOptions& contentOptions,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
      override;

  void doProcessLoadedContent(Tile& tile) override;

  static CesiumAsync::Future<TilesetContentLoaderResult> createLoader(
      const TilesetExternals& externals,
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl,
      bool showCreditsOnScreen);

private:
  void refreshTokenInMainThread();

  TokenRefreshState _refreshTokenState;
  uint32_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
};
} // namespace Cesium3DTilesSelection
