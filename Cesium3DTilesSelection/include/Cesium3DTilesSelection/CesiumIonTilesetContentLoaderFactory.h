#include "TilesetContentLoader.h"
#include "TilesetContentLoaderFactory.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetExternals.h"

#include <CesiumAsync/Future.h>

#include <cstdint>
#include <string>

namespace Cesium3DTilesSelection {
/**
 * @brief A factory for creating a @ref TilesetContentLoader for assets from <a
 * href="https://ion.cesium.com/">Cesium ion</a>.
 */
class CesiumIonTilesetContentLoaderFactory
    : public TilesetContentLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading a Cesium ion asset.
   *
   * @param ionAssetID The Cesium ion asset ID to load.
   * @param ionAccessToken The Cesium ion token to use to authorize requests to
   * this asset.
   * @param ionAssetEndpointUrl The Cesium ion endpoint to use. This can be
   * changed to point to an instance of <a
   * href="https://cesium.com/platform/cesium-ion/cesium-ion-self-hosted/">Cesium
   * ion Self-Hosted</a>.
   */
  CesiumIonTilesetContentLoaderFactory(
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) override;

  virtual bool isValid() const override;

private:
  uint32_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
};
} // namespace Cesium3DTilesSelection