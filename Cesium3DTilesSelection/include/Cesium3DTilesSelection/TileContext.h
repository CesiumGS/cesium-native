#pragma once

#include "BoundingVolume.h"
#include "Cesium3DTilesSelection/CreditSystem.h"

#include <CesiumGeometry/Availability.h>
#include <CesiumGeometry/OctreeAvailability.h>
#include <CesiumGeometry/OctreeTilingScheme.h>
#include <CesiumGeometry/QuadtreeAvailability.h>
#include <CesiumGeometry/QuadtreeRectangleAvailability.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Projection.h>

#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class Tileset;
class TileContext;
/**
 * @brief A tiling context that was created for implicit quadtree or octree
 * tiles.
 *
 * The URLs for the individual tiles are computed from the base URL of the
 * tileset.
 */
class ImplicitTilingContext final {
public:
  /**
   * @brief The templates for the relative URLs of tiles.
   *
   * The template elements of these URLs may be `x`, `y`, or `z` (or `level`),
   * and will be substituted with the corresponding information from
   * a {@link CesiumGeometry::QuadtreeTileID}. The `version` template
   * element will be substituted with the version number of the owning
   * context.
   */
  std::vector<std::string> tileTemplateUrls;

  /**
   * @brief The templates for the relative URLs of the subtree files.
   *
   * The template elements of these URLs may be `x`, `y`, or `z` (or `level`),
   * and will be substituted with the corresponding information from
   * a {@link CesiumGeometry::QuadtreeTileID}. The `version` template
   * element will be substituted with the version number of the owning
   * context.
   */
  std::optional<std::string> subtreeTemplateUrl;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} for this context.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> quadtreeTilingScheme;

  /**
   * @brief The {@link CesiumGeometry::OctreeTilingScheme} for this context.
   */
  std::optional<CesiumGeometry::OctreeTilingScheme> octreeTilingScheme;

  /**
   * @brief The bounding volume of the implicit root tile. This can only be
   * {@link CesiumGeospatial::BoundingRegion} or
   * {@link CesiumGeometry::OrientedBoundingBox}.
   *
   * This will later be use to determine what type of bounding volume to use
   * and how to unproject the implicitly subdivided children.
   */
  BoundingVolume implicitRootBoundingVolume;

  /**
   * @brief The {@link CesiumGeospatial::Projection} for this context.
   *
   * Only relevant if implicitRootBoundingVolume is
   * {@link CesiumGeospatial::BoundingRegion}.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeRectangleAvailability} for this
   * context.
   *
   * Only applicable for quantized-mesh tilesets.
   */
  std::optional<CesiumGeometry::QuadtreeRectangleAvailability>
      rectangleAvailability;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeAvailability} for this
   * context.
   */
  std::optional<CesiumGeometry::QuadtreeAvailability> quadtreeAvailability;

  /**
   * @brief The {@link CesiumGeometry::OctreeAvailability} for this
   * context.
   */
  std::optional<CesiumGeometry::OctreeAvailability> octreeAvailability;

  /**
   * @brief Availability level from the layer.json.
   *
   * If `availabilityLevels` is `n`, then availability information is stored
   * every `n`th level in the tile tree.
   */
  std::optional<int32_t> availabilityLevels;

  /**
   * @brief Any attribution associated with this context/layer.
   */
  std::optional<Credit> credit;
};

/**
 * @brief The action to take for a failed tile.
 */
enum class FailedTileAction {
  /**
   * @brief This failure is considered permanent and this tile should not be
   * retried.
   */
  GiveUp,

  /**
   * @brief This tile should be retried immediately.
   */
  Retry,

  /**
   * @brief This tile should be considered failed for now but possibly retried
   * later.
   */
  Wait
};

/**
 * @brief Signature of {@link FailedTileCallback}.
 */
typedef FailedTileAction FailedTileSignature(Tile& failedTile);

/**
 * @brief A function that serves as a callback for failed tile loading
 * in a {@link TileContext}.
 */
typedef std::function<FailedTileSignature> FailedTileCallback;

/**
 * @brief Signature of {@link ContextInitializerCallback}.
 */
typedef void ContextInitializerSignature(
    const TileContext& parentContext,
    TileContext& currentContext);

/**
 * @brief A function that serves as a callback for initializing a new
 * {@link TileContext} from properties of the parent context.
 */
typedef std::function<ContextInitializerSignature> ContextInitializerCallback;

/**
 * @brief A context in which a {@link Tileset} operates.
 *
 * The context summarizes the information which is needed by a tileset
 * in order to load {@link Tile} data. This includes the base URL
 * that a tileset was loaded from, as well as request headers.
 * The data of individual tiles is obtained by resolving the relative
 * URLs that are obtained from the tiles against the base URL of the
 * context.
 *
 * One tile context is created for each tileset when the initial
 * tileset data is received. When further tiles are loaded or
 * created, they may create additional contexts - for example,
 * for *external* tilesets that generate a whole new context
 * with a new base URL. Each context is added to the set of
 * contexts of the tileset with {@link Tileset::addContext}.
 *
 * Tilesets that contain terrain tiles may additionally create
 * an {@link ImplicitTilingContext}.
 */
class TileContext final {
public:
  /**
   * @brief The {@link Tileset} that this context belongs to.
   */
  Tileset* pTileset = nullptr;

  /**
   * @brief The base URL that the tileset was loaded from.
   */
  std::string baseUrl;

  /**
   * @brief Request headers that are required for requesting tile data.
   *
   * These are pairs of strings of the form (Key, Value) that will
   * be added to the request headers of outgoing requests for tile data.
   */
  std::vector<std::pair<std::string, std::string>> requestHeaders;

  /**
   * @brief The version number of the tileset.
   */
  std::optional<std::string> version;

  /**
   * @brief An {@link ImplicitTilingContext} that may have been
   * created for terrain tilesets.
   */
  std::optional<ImplicitTilingContext> implicitContext;

  /**
   * @brief An optional {@link FailedTileCallback}.
   *
   * This callback will be called when a {@link Tile} goes into the
   * {@link Tile::LoadState `FailedTemporarily`} state, and returns
   * a {@link FailedTileAction} indicating how to react to the
   * failure.
   */
  FailedTileCallback failedTileCallback;

  /**
   * @brief An optional {@link ContextInitializerCallback}.
   *
   * This callback is called once from the main thread in order to initialize
   * this context - which may have been created in a worker thread - from
   * properties of its parent context.
   */
  ContextInitializerCallback contextInitializerCallback;

  /**
   * @brief Another tiling context underlying this one, if any.
   *
   * If a tile is not available from this tiling context, we check the
   * underlyingContext to see if it is available from that one instead. This
   * allows one implicitly-tiled tileset to be layered on top of another one.
   * For example, custom terrain for a small area layered on top of Cesium World
   * Terrain. In this scenario, Cesium World Terrain would be the
   * underlyingContext.
   *
   * This property can be viewed as forming a singly-linked list of contexts.
   * {@link pTopContext} points back to the head of list.
   */
  std::unique_ptr<TileContext> pUnderlyingContext;

  /**
   * @brief Points back to the top context, if this is an underlying context. If
   * this context _is_ the top context, this property is nullptr.
   *
   * {@link pUnderlyingContext} can be viewed as forming a singly-linked list
   * of contexts. This pointer points back to the head of list.
   */
  TileContext* pTopContext = nullptr;
};

} // namespace Cesium3DTilesSelection
