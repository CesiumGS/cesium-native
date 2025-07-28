#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

#include <functional>
#include <memory>
#include <string>

namespace CesiumAsync {

/**
 * @brief An \link IAssetAccessor that wraps another one and handles Cesium ion
 * token refresh when an asset returns a 401 error.
 *
 * It's rarely necessary to use this class directly. It's created by \link
 * Cesium3DTilesSelection::CesiumIonTilesetContentLoaderFactory and \link
 * CesiumRasterOverlays::IonRasterOverlay as needed.
 */
class CesiumIonAssetAccessor
    : public std::enable_shared_from_this<CesiumIonAssetAccessor>,
      public IAssetAccessor {
public:
  struct UpdatedToken {
    std::string token;
    std::string authorizationHeader;
  };

  CesiumIonAssetAccessor(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<IAssetAccessor>& pAggregatedAccessor,
      const std::string& assetEndpointUrl,
      const std::vector<IAssetAccessor::THeader>& assetEndpointHeaders,
      std::function<Future<void>(const UpdatedToken&)> updatedTokenCallback);

  Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override;

  Future<std::shared_ptr<IAssetRequest>> request(
      const AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const std::span<const std::byte>& contentPayload = {}) override;

  void tick() noexcept override;

  void notifyOwnerIsBeingDestroyed();

private:
  SharedFuture<UpdatedToken> refreshTokenInMainThread(
      const AsyncSystem& asyncSystem,
      const std::string& currentAuthorizationHeader,
      const std::string& currentAccessTokenQueryParameterValue);

  std::shared_ptr<spdlog::logger> _pLogger;
  std::shared_ptr<IAssetAccessor> _pAggregatedAccessor;
  std::string _assetEndpointUrl;
  std::vector<IAssetAccessor::THeader> _assetEndpointHeaders;
  std::optional<std::function<Future<void>(const UpdatedToken&)>>
      _updatedTokenCallback;
  std::optional<SharedFuture<UpdatedToken>> _tokenRefreshInProgress;
};

} // namespace CesiumAsync
