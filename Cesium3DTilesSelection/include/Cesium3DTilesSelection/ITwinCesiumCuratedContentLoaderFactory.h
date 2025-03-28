#include "TilesetContentLoader.h"
#include "TilesetContentLoaderFactory.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetExternals.h"

#include <CesiumAsync/Future.h>

#include <cstdint>
#include <string>

namespace Cesium3DTilesSelection {
/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/cesium-curated-content/overview/">iTwin
 * Cesium Curated Content</a> API.
 */
class ITwinCesiumCuratedContentLoaderFactory
    : public TilesetContentLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading content from iTwin Cesium Curated
   * Content.
   *
   * @param iTwinCesiumContentID The ID of the item to load.
   * @param iTwinAccessToken The access token to use to access the API.
   */
  ITwinCesiumCuratedContentLoaderFactory(
      uint32_t iTwinCesiumContentID,
      const std::string& iTwinAccessToken);

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) override;

  virtual bool isValid() const override;

private:
  uint32_t _iTwinCesiumContentID;
  std::string _iTwinAccessToken;
};
} // namespace Cesium3DTilesSelection