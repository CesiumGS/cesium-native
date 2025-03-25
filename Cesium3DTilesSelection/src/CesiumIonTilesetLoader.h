#pragma once

#include "CesiumAsync/IAssetAccessor.h"

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>

#include <functional>
#include <string>

namespace Cesium3DTilesSelection {

class CesiumIonTilesetLoader;

class EndpointResource {
public:
  virtual bool needsAuthHeaderOnInitialRequest() const = 0;

  virtual std::string getUrl(
      int64_t assetID,
      const std::string& accessToken,
      const std::string& assetEndpointUrl) const = 0;
  virtual ~EndpointResource() = default;
};

// This is an IAssetAccessor decorator that handles token refresh for any asset
// that returns a 401 error.
class CesiumIonAssetAccessor
    : public std::enable_shared_from_this<CesiumIonAssetAccessor>,
      public CesiumAsync::IAssetAccessor {
public:
  CesiumIonAssetAccessor(
      CesiumIonTilesetLoader& tilesetLoader,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAggregatedAccessor,
      const std::shared_ptr<EndpointResource>& endpointResource)
      : _pTilesetLoader(&tilesetLoader),
        _pAggregatedAccessor(pAggregatedAccessor),
        _endpointResource(endpointResource) {}

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override;

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const std::span<const std::byte>& contentPayload = {}) override;

  void tick() noexcept override;

  void notifyLoaderIsBeingDestroyed();

private:
  CesiumIonTilesetLoader* _pTilesetLoader;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAggregatedAccessor;
  std::shared_ptr<EndpointResource> _endpointResource;
};

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

private:
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

  CesiumAsync::SharedFuture<std::string> refreshTokenInMainThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& currentAuthorizationHeader,
      const EndpointResource& endpointResource);

protected:
  virtual std::shared_ptr<CesiumIonAssetAccessor> createAssetAccessor(
      CesiumIonTilesetLoader& tilesetLoader,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAggregatedAccessor);

  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl,
      std::unique_ptr<EndpointResource>&& endpointResource,
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
      std::unique_ptr<EndpointResource>&& endpointResource,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

private:
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
