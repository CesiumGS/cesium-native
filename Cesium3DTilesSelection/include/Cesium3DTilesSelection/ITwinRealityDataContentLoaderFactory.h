#include "TilesetContentLoader.h"
#include "TilesetContentLoaderFactory.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetExternals.h"

#include <CesiumAsync/Future.h>

#include <string>

namespace Cesium3DTilesSelection {
/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/reality-management/overview/">iTwin
 * Reality Management</a> API.
 */
class ITwinRealityDataContentLoaderFactory
    : public TilesetContentLoaderFactory {
public:
  /**
   * @brief Callback to obtain a new access token for the iTwin API. Receives
   * the previous access token as parameter.
   */
  using TokenRefreshCallback =
      std::function<CesiumAsync::Future<CesiumUtility::Result<std::string>>(
          const std::string&)>;

  /**
   * @brief Creates a new factory for loading iTwin reality data.
   *
   * @param realityDataId The ID of the reality data to load.
   * @param iTwinId @parblock
   * The ID of the iTwin this reality data is associated with.
   *
   * As described in the <a
   * href="https://developer.bentley.com/apis/reality-management/operations/get-read-access-to-reality-data-container/">iTwin
   * Reality Management API documentation</a>:
   *
   * > The `iTwinId` parameter is optional, but it is preferable to provide it,
   * > because the permissions used to access the container are determined from
   * > the iTwin. With no iTwin specified, the operation can succeed in some
   * > cases (e.g. the user is the reality data's owner), but we recommend to
   * > provide a `iTwinId`.
   *
   * @endparblock
   * @param iTwinAccessToken The access token to use to access the API.
   * @param tokenRefreshCallback Callback that will be called to obtain a new
   * access token when the provided one has expired.
   */
  ITwinRealityDataContentLoaderFactory(
      const std::string& realityDataId,
      const std::optional<std::string>& iTwinId,
      const std::string& iTwinAccessToken,
      TokenRefreshCallback&& tokenRefreshCallback);

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) override;

  virtual bool isValid() const override;

private:
  std::string _realityDataId;
  std::optional<std::string> _iTwinId;
  std::string _iTwinAccessToken;
  TokenRefreshCallback _tokenRefreshCallback;
};
} // namespace Cesium3DTilesSelection