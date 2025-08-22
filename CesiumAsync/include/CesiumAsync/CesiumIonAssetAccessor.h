#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace CesiumAsync {

/**
 * @brief An \ref IAssetAccessor that wraps another one and handles Cesium ion
 * token refresh when an asset returns a 401 error.
 *
 * It's rarely necessary to use this class directly. It's created by \ref
 * Cesium3DTilesSelection::CesiumIonTilesetContentLoaderFactory and \ref
 * CesiumRasterOverlays::IonRasterOverlay as needed.
 */
class CesiumIonAssetAccessor
    : public std::enable_shared_from_this<CesiumIonAssetAccessor>,
      public IAssetAccessor {
public:
  /**
   * @brief The details of the updated token.
   */
  struct UpdatedToken {
    /**
     * @brief The new token.
     */
    std::string token;

    /**
     * @brief The new Authorization header containing the new token.
     */
    std::string authorizationHeader;
  };

  /**
   * @brief Creates a new instance.
   */
  CesiumIonAssetAccessor(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<IAssetAccessor>& pAggregatedAccessor,
      const std::string& assetEndpointUrl,
      const std::vector<IAssetAccessor::THeader>& assetEndpointHeaders,
      std::function<Future<void>(const UpdatedToken&)> updatedTokenCallback);

  /**
   * \inheritdoc
   */
  Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override;

  /**
   * \inheritdoc
   */
  Future<std::shared_ptr<IAssetRequest>> request(
      const AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const std::span<const std::byte>& contentPayload = {}) override;

  /**
   * \inheritdoc
   */
  void tick() noexcept override;

  /**
   * @brief Notifies this accessor that it's owner has been destroyed. When the
   * owner is destroyed, the token will no longer be refreshed.
   */
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
      _maybeUpdatedTokenCallback;
  std::optional<SharedFuture<UpdatedToken>> _tokenRefreshInProgress;
};

} // namespace CesiumAsync
