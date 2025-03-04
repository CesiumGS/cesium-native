#pragma once

#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>

#include <functional>
#include <string>

namespace Cesium3DTilesSelection {

class CesiumIonAssetAccessor;

class CesiumIonTilesetLoader : public TilesetContentLoader {
public:
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  CesiumIonTilesetLoader(
      int64_t ionAssetID,
      std::string&& ionAccessToken,
      std::string&& ionAssetEndpointUrl,
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader,
      AuthorizationHeaderChangeListener&& headerChangeListener,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  virtual ~CesiumIonTilesetLoader() noexcept;

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid) override;

  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  refreshTokenIfNeeded(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

private:
  CesiumAsync::SharedFuture<std::string> refreshTokenInMainThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& currentAuthorizationHeader);

  CesiumGeospatial::Ellipsoid _ellipsoid;
  int64_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
  AuthorizationHeaderChangeListener _headerChangeListener;
  std::shared_ptr<spdlog::logger> _pLogger;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pTilesetAccessor;
  std::shared_ptr<CesiumIonAssetAccessor> _pIonAccessor;
  std::optional<CesiumAsync::SharedFuture<std::string>> _tokenRefreshInProgress;

  friend class CesiumIonAssetAccessor;
};
} // namespace Cesium3DTilesSelection
