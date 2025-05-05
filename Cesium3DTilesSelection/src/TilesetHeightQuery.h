#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumUtility/IntrusivePointer.h>

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

  ~TilesetHeightQuery();

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
   * @brief The ellipsoid on which the input position is defined.
   */
  CesiumGeospatial::Ellipsoid ellipsoid;

  /**
   * @brief The current intersection of the ray with the tileset. If there are
   * multiple intersections, this will be the one closest to the origin of the
   * ray.
   */
  std::optional<CesiumGltfContent::GltfUtilities::RayGltfHit> intersection;

  /**
   * @brief Non-leaf tiles with additive refinement whose bounding volumes are
   * intersected by the query ray.
   */
  std::vector<Tile::Pointer> additiveCandidateTiles;

  /**
   * @brief The current set of leaf tiles whose bounding volumes are intersected
   * by the query ray.
   */
  std::vector<Tile::Pointer> candidateTiles;

  /**
   * @brief The previous set of leaf tiles. Swapping `candidateTiles` and
   * `previousCandidateTiles` each frame allows us to avoid a heap allocation
   * for a new vector each frame.
   */
  std::vector<Tile::Pointer> previousCandidateTiles;

  /**
   * @brief Find the intersection of the ray with the given tile. If there is
   * one, and if it's closer to the ray's origin than the previous best-known
   * intersection, then {@link TilesetHeightQuery::intersection} will be
   * updated.
   *
   * @param pTile The tile to test for intersection with the ray.
   * @param outWarnings On return, reports any warnings that occurred while
   * attempting to intersect the ray with the tile.
   */
  void intersectVisibleTile(Tile* pTile, std::vector<std::string>& outWarnings);

  /**
   * @brief Find candidate tiles for the height query by traversing the tile
   * tree, starting with the given tile.
   *
   * Any tile whose bounding volume intersects the ray will be added to the
   * {@link TilesetHeightQuery::candidateTiles} vector. Non-leaf tiles that are
   * additively-refined will be added to
   * {@link TilesetHeightQuery::additiveCandidateTiles}.
   *
   * @param pTile The tile at which to start traversal.
   * @param outWarnings On return, reports any warnings that occurred during
   * candidate search.
   */
  void findCandidateTiles(Tile* pTile, std::vector<std::string>& outWarnings);
};

/**
 * @brief A request for a batch of height queries. When all of the queries are
 * complete, they will be delivered to the requestor via resolving a promise.
 */
struct TilesetHeightRequest : public TileLoadRequester {
  TilesetHeightRequest(
      std::vector<TilesetHeightQuery>&& queries,
      const CesiumAsync::Promise<SampleHeightResult>& promise) noexcept;
  TilesetHeightRequest(const TilesetHeightRequest& rhs) noexcept = delete;
  TilesetHeightRequest(TilesetHeightRequest&& rhs) noexcept;
  virtual ~TilesetHeightRequest() noexcept;

  /**
   * @brief The individual height queries in this request.
   */
  std::vector<TilesetHeightQuery> queries;

  /**
   * @brief The promise to be resolved when all height queries are complete.
   */
  CesiumAsync::Promise<SampleHeightResult> promise;

  /**
   * @brief The set of tiles that need to be loaded in order for this height
   * request to be completed.
   */
  std::set<Tile*> tilesToLoad;

  /**
   * @brief Process a given list of height requests. This is called by the {@link Tileset}
   * in every call to {@link Tileset::updateView}.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param contentManager The content manager.
   * @param options Options associated with the tileset.
   * @param heightRequests The list of all height requests. Completed requests
   * will be removed from this list.
   */
  static void processHeightRequests(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TilesetContentManager& contentManager,
      const TilesetOptions& options,
      std::list<TilesetHeightRequest>& heightRequests);

  double getWeight() const override;

  /** @inheritdoc */
  bool hasMoreTilesToLoadInWorkerThread() const override;
  /** @inheritdoc */
  Tile* getNextTileToLoadInWorkerThread() override;

  /** @inheritdoc */
  bool hasMoreTilesToLoadInMainThread() const override;
  /** @inheritdoc */
  Tile* getNextTileToLoadInMainThread() override;

  /**
   * @brief Cancels all outstanding height requests and rejects the associated
   * futures. This is useful when it is known that the height requests will
   * never complete, such as when the tileset fails to load or when it is being
   * destroyed.
   *
   * @param heightRequests The height requests to cancel.
   * @param message The message explaining what went wrong.
   */
  static void failHeightRequests(
      std::list<TilesetHeightRequest>& heightRequests,
      const std::string& message);

  /**
   * @brief Tries to complete this height request. Returns false if further data
   * still needs to be loaded and thus the request cannot yet complete.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param contentManager The content manager.
   * @param options Options associated with the tileset.
   */
  bool tryCompleteHeightRequest(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TilesetContentManager& contentManager,
      const TilesetOptions& options);
};

} // namespace Cesium3DTilesSelection
