#pragma once

#include "Cesium3DTilesSelection/Library.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class ITileExcluder;

/**
 * @brief Options for configuring the parsing of a {@link Tileset}'s content
 * and construction of Gltf models.
 */
struct CESIUM3DTILESSELECTION_API TilesetContentOptions {
  /**
   * @brief Whether to include a water mask within the Gltf extras.
   *
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   */
  bool enableWaterMask = false;

  /**
   * @brief Whether to generate smooth normals when normals are missing in the
   * original Gltf.
   *
   * According to the Gltf spec: "When normals are not specified, client
   * implementations should calculate flat normals." However, calculating flat
   * normals requires duplicating vertices. This option allows the gltfs to be
   * sent with explicit smooth normals when the original gltf was missing
   * normals.
   */
  bool generateMissingNormalsSmooth = false;
};

/**
 * @brief Defines the fog density at a certain height.
 *
 * @see TilesetOptions::fogDensityTable
 */
struct CESIUM3DTILESSELECTION_API FogDensityAtHeight {

  /** @brief The height. */
  double cameraHeight;

  /** @brief The fog density. */
  double fogDensity;
};

/**
 * @brief Additional options for configuring a {@link Tileset}.
 */
struct CESIUM3DTILESSELECTION_API TilesetOptions {
  /**
   * @brief A credit text for this tileset, if needed.
   */
  std::optional<std::string> credit;

  /**
   * @brief The maximum number of pixels of error when rendering this tileset.
   * This is used to select an appropriate level-of-detail.
   *
   * When a tileset uses the older layer.json / quantized-mesh format rather
   * than 3D Tiles, this value is effectively divided by 8.0. So the default
   * value of 16.0 corresponds to the standard value for quantized-mesh terrain
   * of 2.0.
   */
  double maximumScreenSpaceError = 16.0;

  /**
   * @brief The maximum number of tiles that may simultaneously be in the
   * process of loading.
   */
  uint32_t maximumSimultaneousTileLoads = 20;

  /**
   * @brief Indicates whether the ancestors of rendered tiles should be
   * preloaded. Setting this to true optimizes the zoom-out experience and
   * provides more detail in newly-exposed areas when panning. The down side is
   * that it requires loading more tiles.
   */
  bool preloadAncestors = true;

  /**
   * @brief Indicates whether the siblings of rendered tiles should be
   * preloaded. Setting this to true causes tiles with the same parent as a
   * rendered tile to be loaded, even if they are culled. Setting this to true
   * may provide a better panning experience at the cost of loading more tiles.
   */
  bool preloadSiblings = true;

  /**
   * @brief The number of loading descendant tiles that is considered "too
   * many". If a tile has too many loading descendants, that tile will be loaded
   * and rendered before any of its descendants are loaded and rendered. This
   * means more feedback for the user that something is happening at the cost of
   * a longer overall load time. Setting this to 0 will cause each tile level to
   * be loaded successively, significantly increasing load time. Setting it to a
   * large number (e.g. 1000) will minimize the number of tiles that are loaded
   * but tend to make detail appear all at once after a long wait.
   */
  uint32_t loadingDescendantLimit = 20;

  /**
   * @brief Never render a tileset with missing tiles.
   *
   * When true, the tileset will guarantee that the tileset will never be
   * rendered with holes in place of tiles that are not yet loaded. It does this
   * by refusing to refine a parent tile until all of its child tiles are ready
   * to render. Thus, when the camera moves, we will always have something -
   * even if it's low resolution - to render any part of the tileset that
   * becomes visible. When false, overall loading will be faster, but
   * newly-visible parts of the tileset may initially be blank.
   */
  bool forbidHoles = false;

  /**
   * @brief Enable culling of tiles against the frustum.
   */
  bool enableFrustumCulling = true;

  /**
   * @brief Enable culling of tiles that cannot be seen through atmospheric fog.
   */
  bool enableFogCulling = true;

  /**
   * @brief Whether culled tiles should be refined until they meet
   * culledScreenSpaceError.
   *
   * When true, any culled tile from a disabled culling stage will be refined
   * until it meets the specified culledScreenSpaceError. Otherwise, its
   * screen-space error check will be disabled altogether and it will not bother
   * to refine any futher.
   */
  bool enforceCulledScreenSpaceError = true;

  /**
   * @brief The screen-space error to refine until for culled tiles from
   * disabled culling stages.
   *
   * When enforceCulledScreenSpaceError is true, culled tiles from disabled
   * culling stages will be refined until they meet this screen-space error
   * value.
   */
  double culledScreenSpaceError = 64.0;

  /**
   * @brief The maximum number of bytes that may be cached.
   *
   * Note that this value, even if 0, will never
   * cause tiles that are needed for rendering to be unloaded. However, if the
   * total number of loaded bytes is greater than this value, tiles will be
   * unloaded until the total is under this number or until only required tiles
   * remain, whichever comes first.
   */
  int64_t MaximumCachedBytes = 256 * 1024 * 1024;

  /**
   * @brief A table that maps the camera height above the ellipsoid to a fog
   * density. Tiles that are in full fog are culled. The density of the fog
   * increases as this number approaches 1.0 and becomes less dense as it
   * approaches zero. The more dense the fog is, the more aggressively the tiles
   * are culled. For example, if the camera is a height of 1000.0m above the
   * ellipsoid, increasing the value to 3.0e-3 will cause many tiles close to
   * the viewer be culled. Decreasing the value will push the fog further from
   * the viewer, but decrease performance as more of the tiles are rendered.
   * Tiles are culled when `1.0 - glm::exp(-(distance * distance * fogDensity *
   * fogDensity))` is >= 1.0.
   */
  std::vector<FogDensityAtHeight> fogDensityTable = {
      {359.393, 2.0e-5},     {800.749, 2.0e-4},     {1275.6501, 1.0e-4},
      {2151.1192, 7.0e-5},   {3141.7763, 5.0e-5},   {4777.5198, 4.0e-5},
      {6281.2493, 3.0e-5},   {12364.307, 1.9e-5},   {15900.765, 1.0e-5},
      {49889.0549, 8.5e-6},  {78026.8259, 6.2e-6},  {99260.7344, 5.8e-6},
      {120036.3873, 5.3e-6}, {151011.0158, 5.2e-6}, {156091.1953, 5.1e-6},
      {203849.3112, 4.2e-6}, {274866.9803, 4.0e-6}, {319916.3149, 3.4e-6},
      {493552.0528, 2.6e-6}, {628733.5874, 2.2e-6}, {1000000.0, 0.0}};

  /**
   * @brief Whether to render tiles directly under the camera, even if they're
   * not in the view frustum.
   *
   * This is useful for detecting the camera's collision with terrain and other
   * models. NOTE: This option currently only works with tiles that use a
   * `region` as their bounding volume. It is ignored for other bounding volume
   * types.
   */
  bool renderTilesUnderCamera = true;

  /**
   * @brief A list of interfaces that are given an opportunity to exclude tiles
   * from loading and rendering. If any of the excluders indicate that a tile
   * should not be loaded, it will not be loaded.
   */
  std::vector<std::shared_ptr<ITileExcluder>> excluders;

  /**
   * @brief Options for configuring the parsing of a {@link Tileset}'s content
   * and construction of Gltf models.
   */
  TilesetContentOptions contentOptions;
};

} // namespace Cesium3DTilesSelection
