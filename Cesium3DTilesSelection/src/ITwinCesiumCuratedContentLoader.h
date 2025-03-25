#pragma once

#include "CesiumIonTilesetLoader.h"

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>

#include <functional>
#include <string>

namespace Cesium3DTilesSelection {

class ITwinCesiumCuratedContentLoader : public CesiumIonTilesetLoader {
public:
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  static CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);
};
} // namespace Cesium3DTilesSelection
