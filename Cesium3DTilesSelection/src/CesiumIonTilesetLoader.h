#pragma once

#include "CesiumAsync/IAssetAccessor.h"

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
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
      std::string&& url,
      std::string&& ionAccessToken,
      bool needsAuthHeader,
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader,
      AuthorizationHeaderChangeListener&& headerChangeListener,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID)
      : _ellipsoid(ellipsoid),
        _url(std::move(url)),
        _ionAccessToken(std::move(ionAccessToken)),
        _needsAuthHeader(needsAuthHeader),
        _pAggregatedLoader(std::move(pAggregatedLoader)),
        _headerChangeListener(std::move(headerChangeListener)),
        _pLogger(nullptr),
        _pTilesetAccessor(nullptr),
        _pIonAccessor(nullptr),
        _tokenRefreshInProgress() {}

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

private:
  CesiumAsync::SharedFuture<std::string> refreshTokenInMainThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& currentAuthorizationHeader);

protected:
  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const std::string& url,
      const std::string& ionAccessToken,
      bool needsAuthHeader,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  refreshTokenIfNeeded(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const std::string& url,
      const std::string& ionAccessToken,
      bool needsAuthHeader,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

private:
  CesiumGeospatial::Ellipsoid _ellipsoid;
  std::string _url;
  std::string _ionAccessToken;
  bool _needsAuthHeader;
  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
  AuthorizationHeaderChangeListener _headerChangeListener;
  std::shared_ptr<spdlog::logger> _pLogger;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pTilesetAccessor;
  std::shared_ptr<CesiumIonAssetAccessor> _pIonAccessor;
  std::optional<CesiumAsync::SharedFuture<std::string>> _tokenRefreshInProgress;

  friend class CesiumIonAssetAccessor;
};
} // namespace Cesium3DTilesSelection
