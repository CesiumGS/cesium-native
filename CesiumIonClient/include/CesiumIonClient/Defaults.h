#pragma once

#include <cstdint>
#include <vector>

namespace CesiumIonClient {

/**
 * @brief The default assets.
 */
struct DefaultAssets {
  /**
   * @brief The asset id of the default imagery asset.
   */
  int64_t imagery = -1;

  /**
   * @brief The asset id of the default terrain asset.
   */
  int64_t terrain = -1;

  /**
   * @brief The asset id of the default buildings asset.
   */
  int64_t buildings = -1;
};

/**
 * @brief A raster overlay available for use with a quick add asset.
 */
struct QuickAddRasterOverlay {
  /**
   * @brief The name of this asset.
   */
  std::string name{};

  /**
   * @brief The unique identifier for this asset.
   */
  int64_t assetId = -1;

  /**
   * @brief `true` if the user is subscribed to the asset, `false` otherwise.
   */
  bool subscribed = false;
};

/**
 * @brief A quick add asset.
 */
struct QuickAddAsset {
  /**
   * @brief The name of this asset.
   */
  std::string name{};

  /**
   * @brief The name of the main asset. In most cases this will be the same as
   * `name`, but in the cases of assets with raster overlays, this will be the
   * non-imagery asset.
   */
  std::string objectName{};

  /**
   * @brief A Markdown compatible string describing this asset.
   */
  std::string description{};

  /**
   * @brief The unique identifier for this asset.
   */
  int64_t assetId = -1;

  /**
   * @brief This asset's type.
   *
   * Valid values are: `3DTILES`, `GLTF`, `IMAGERY`, `TERRAIN`, `KML`, `CZML`,
   * `GEOJSON`.
   */
  std::string type{};

  /**
   * @brief `true` if the user is subscribed to the asset, `false` otherwise.
   */
  bool subscribed = false;

  /**
   * @brief The raster overlays available for this asset
   */
  std::vector<QuickAddRasterOverlay> rasterOverlays{};
};

/**
 * @brief The data returned by Cesium ion's `v1/defaults` service. It includes
 * information about default imagery, terrain and building assets along with
 * quick add assets that can be useful to use within other applications.
 */
struct Defaults {
  /**
   * @brief The default assets
   */
  DefaultAssets defaultAssets{};

  /**
   * @brief The quick add assets
   */
  std::vector<QuickAddAsset> quickAddAssets{};
};

} // namespace CesiumIonClient
