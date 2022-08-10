#pragma once

#include "TilesetContentLoader.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/TilesetExternals.h>

#include <functional>
#include <string>

namespace Cesium3DTilesSelection {
class CesiumIonTilesetLoader : public TilesetContentLoader {
  enum class TokenRefreshState { None, Loading, Done, Failed };

public:
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  CesiumIonTilesetLoader(
      uint32_t ionAssetID,
      std::string&& ionAccessToken,
      std::string&& ionAssetEndpointUrl,
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader,
      AuthorizationHeaderChangeListener&& headerChangeListener);

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(const Tile& tile) override;

  static CesiumAsync::Future<TilesetContentLoaderResult> createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen);

private:
  void refreshTokenInMainThread(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const CesiumAsync::AsyncSystem& asyncSystem);

  TokenRefreshState _refreshTokenState;
  uint32_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
  AuthorizationHeaderChangeListener _headerChangeListener;
};
} // namespace Cesium3DTilesSelection
