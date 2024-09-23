#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <list>
#include <set>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class TilesetContentManager;
struct TilesetOptions;
struct SampleHeightResult;

class TilesetHeightQuery {
public:
  /**
   * @brief Initializes a new instance.
   *
   * @param position The position at which to query a height. The existing
   * height is ignored.
   * @param ellipsoid The ellipsoid on which the position is defined.
   */
  TilesetHeightQuery(
      const CesiumGeospatial::Cartographic& position,
      const CesiumGeospatial::Ellipsoid& ellipsoid);

  /**
   * @brief The original input position for which the height is to be queried.
   */
  CesiumGeospatial::Cartographic inputPosition;

  /**
   * @brief A ray created from the {@link TilesetHeightQuery::inputPosition}.
   *
   */
  CesiumGeometry::Ray ray;

  /**
   * @brief The current intersection of the ray with the tileset. If there are
   * multiple intersections, this will be the one closest to the origin of the
   * ray.
   */
  std::optional<CesiumGltfContent::GltfUtilities::RayGltfHit> intersection;

  /**
   * @brief The query ray intersect the bounding volume of these non-leaf tiles
   * that are still relevant because of additive refinement.
   */
  std::vector<Tile*> additiveCandidateTiles;

  /**
   * @brief The current set of leaf tiles whose bounding volume the query ray
   * passes through.
   */
  std::vector<Tile*> candidateTiles;

  /**
   * @brief The previous set of leaf files. Swapping `candidateTiles` and
   * `previousCandidateTiles` each frame allows us to avoid a heap allocation
   * for a new vector each frame.
   */
  std::vector<Tile*> previousCandidateTiles;

  /**
   * @brief Find the intersection of the ray with the given tile. If there is
   * one, and it is closer to the origin of the ray than the previous
   * best-known intersection, then {@link TilesetHeightQuery::intersection}
   * will be updated.
   *
   * @param pTile The tile to test for intersection with the ray.
   * @param outWarnings On return, any warnings that occurred while attempting
   * to intersect the ray with the given tile.
   */
  void intersectVisibleTile(Tile* pTile, std::vector<std::string>& outWarnings);

  /**
   * @brief Find the candidate tiles for query by traversing the tile tree
   * starting with a given tile.
   *
   * Any tile whose bounding volume intersects the ray will be added to the
   * {@link TilesetHeightQuery::candidateTiles} vector. Non-leaf tiles that are
   * additively-refined will be added to
   * {@link TilesetHeightQuery::additiveCandidateTiles}.
   *
   * @param pTile The tile at which to start traversal.
   * @param loadedTiles The linked list of loaded tiles, used to ensure that
   * tiles loaded for height queries stay loaded long enough to complete the
   * query and no longer.
   * @param outWarnings On return, any warnings that occurred during candidate
   * search.
   */
  void findCandidateTiles(
      Tile* pTile,
      Tile::LoadedLinkedList& loadedTiles,
      std::vector<std::string>& outWarnings);
};

/**
 * @brief A request for a batch of height queries. When all of the queries are
 * complete, they will be delivered to the requestor via resolving a promise.
 */
struct TilesetHeightRequest {
  /**
   * @brief The individual height queries in this request.
   */
  std::vector<TilesetHeightQuery> queries;

  /**
   * @brief The promise to be resolved when all height queries are complete.
   */
  CesiumAsync::Promise<SampleHeightResult> promise;

  /**
   * @brief Process a given list of height requests. This is called by the {@link Tileset}
   * every call to {@link Tileset::updateView}.
   *
   * @param contentManager The content manager.
   * @param options Options associated with the tileset.
   * @param loadedTiles The linked list of loaded tiles, used to ensure that
   * tiles loaded for height queries stay loaded long enough to complete the
   * query and no longer.
   * @param heightRequests The list of all height requests. Completed requests
   * will be removed from this list.
   * @param heightQueryLoadQueue Tiles that still need to be loaded before all
   * height requests can complete are added to this vector.
   */
  static void processHeightRequests(
      TilesetContentManager& contentManager,
      const TilesetOptions& options,
      Tile::LoadedLinkedList& loadedTiles,
      std::list<TilesetHeightRequest>& heightRequests,
      std::vector<Tile*>& heightQueryLoadQueue);

  /**
   * @brief Cancels all outstanding height requests and rejects the associated
   * futures. This is useful when it is known that the height requests will
   * never complete, such as when the tileset fails to load.
   *
   * @param heightRequests The height requests to cancel.
   * @param message The message explaining what went wrong.
   */
  static void failHeightRequests(
      std::list<TilesetHeightRequest>& heightRequests,
      const std::string& message);

  /**
   * @brief Tries to complete this height request. Returns false if further data
   * still needs to be loaded and so the request cannot be completed yet.
   *
   * @param contentManager The content manager.
   * @param options Options associated with the tileset.
   * @param loadedTiles The linked list of loaded tiles, used to ensure that
   * tiles loaded for height queries stay loaded long enough to complete the
   * query and no longer.
   * @param tilesNeedingLoading Tiles that needs to be loaded before this height
   * request can complete.
   */
  bool tryCompleteHeightRequest(
      TilesetContentManager& contentManager,
      const TilesetOptions& options,
      Tile::LoadedLinkedList& loadedTiles,
      std::set<Tile*>& tilesNeedingLoading);
};

} // namespace Cesium3DTilesSelection
