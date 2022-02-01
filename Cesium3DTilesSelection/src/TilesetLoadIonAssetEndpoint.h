#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <CesiumAsync/Future.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Loads an asset's endpoint information from Cesium ion and triggers
 * {@link Tileset::LoadTilesetDotJson} at the asset's endpoint URL.
 */
class Tileset::LoadIonAssetEndpoint {
public:
  static CesiumAsync::Future<void> start(Tileset& tileset);

private:
  struct Private;
};

} // namespace Cesium3DTilesSelection
