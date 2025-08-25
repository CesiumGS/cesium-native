#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumUtility/Result.h>

#include <spdlog/spdlog.h>

#include <functional>
#include <string>

namespace Cesium3DTilesSelection {

class RealityDataAssetAccessor;

class ITwinRealityDataContentLoader : public TilesetContentLoader {
public:
  /**
   * @brief Callback to obtain a new access token for the iTwin API. Receives
   * the previous access token as parameter.
   */
  using TokenRefreshCallback =
      std::function<CesiumAsync::Future<CesiumUtility::Result<std::string>>(
          const std::string&)>;

  ITwinRealityDataContentLoader(
      const std::string& accessToken,
      TokenRefreshCallback&& tokenRefreshCallback,
      std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader);
  ~ITwinRealityDataContentLoader();

  static CesiumAsync::Future<
      TilesetContentLoaderResult<ITwinRealityDataContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const std::string& realityDataId,
      const std::optional<std::string>& iTwinId,
      const std::string& iTwinAccessToken,
      TokenRefreshCallback&& tokenRefreshCallback,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid) override;

private:
  CesiumAsync::Future<std::string> obtainNewAccessToken();

  std::unique_ptr<TilesetContentLoader> _pAggregatedLoader;
  std::shared_ptr<RealityDataAssetAccessor> _pRealityDataAccessor;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pTilesetAccessor;
  std::shared_ptr<spdlog::logger> _pLogger;
  std::string _iTwinAccessToken;
  TokenRefreshCallback _tokenRefreshCallback;

  friend class RealityDataAssetAccessor;
};
} // namespace Cesium3DTilesSelection
