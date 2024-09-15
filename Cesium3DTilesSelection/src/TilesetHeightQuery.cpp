#include "TilesetHeightQuery.h"

#include "TileUtilities.h"
#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/SampleHeightResult.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltfContent/GltfUtilities.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumUtility;
using namespace CesiumAsync;

namespace {
bool boundingVolumeContainsCoordinate(
    const BoundingVolume& boundingVolume,
    const Ray& ray,
    const Cartographic& coordinate) {
  struct Operation {
    const Ray& ray;
    const Cartographic& coordinate;

    bool operator()(const OrientedBoundingBox& boundingBox) noexcept {
      std::optional<double> t =
          IntersectionTests::rayOBBParametric(ray, boundingBox);
      return t && t.value() >= 0;
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return boundingRegion.getRectangle().contains(coordinate);
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      std::optional<double> t =
          IntersectionTests::raySphereParametric(ray, boundingSphere);
      return t && t.value() >= 0;
    }

    bool operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return boundingRegion.getBoundingRegion().getRectangle().contains(
          coordinate);
    }

    bool operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return s2Cell.computeBoundingRegion().getRectangle().contains(coordinate);
    }
  };

  return std::visit(Operation{ray, coordinate}, boundingVolume);
}

// The ray for height queries starts at this fraction of the ellipsoid max
// radius above the ellipsoid surface. If a tileset surface is more than this
// distance above the ellipsoid, it may be missed by height queries.
// 0.007 is chosen to accomodate Olympus Mons, the tallest peak on Mars. 0.007
// is seven-tenths of a percent, or about 44,647 meters for WGS84, well above
// the highest point on Earth.
const double rayOriginHeightFraction = 0.007;

Ray createRay(const Cartographic& position, const Ellipsoid& ellipsoid) {
  Cartographic startPosition(
      position.longitude,
      position.latitude,
      ellipsoid.getMaximumRadius() * rayOriginHeightFraction);

  return Ray(
      Ellipsoid::WGS84.cartographicToCartesian(startPosition),
      -Ellipsoid::WGS84.geodeticSurfaceNormal(startPosition));
}

} // namespace

TilesetHeightQuery::TilesetHeightQuery(
    const Cartographic& position,
    const Ellipsoid& ellipsoid)
    : inputPosition(position),
      ray(createRay(position, ellipsoid)),
      intersection(),
      additiveCandidateTiles(),
      candidateTiles(),
      previousCandidateTiles() {}

void TilesetHeightQuery::intersectVisibleTile(
    Tile* pTile,
    std::vector<std::string>& outWarnings) {
  TileRenderContent* pRenderContent = pTile->getContent().getRenderContent();
  if (!pRenderContent)
    return;

  auto gltfIntersectResult =
      CesiumGltfContent::GltfUtilities::intersectRayGltfModel(
          this->ray,
          pRenderContent->getModel(),
          true,
          pTile->getTransform());

  if (!gltfIntersectResult.warnings.empty()) {
    outWarnings.insert(
        outWarnings.end(),
        std::make_move_iterator(gltfIntersectResult.warnings.begin()),
        std::make_move_iterator(gltfIntersectResult.warnings.end()));
  }

  // Set ray info to this hit if closer, or the first hit
  if (!this->intersection.has_value()) {
    this->intersection = std::move(gltfIntersectResult.hit);
  } else {
    double prevDistSq = this->intersection->rayToWorldPointDistanceSq;
    double thisDistSq = intersection->rayToWorldPointDistanceSq;
    if (thisDistSq < prevDistSq)
      this->intersection = std::move(gltfIntersectResult.hit);
  }
}

void TilesetHeightQuery::findCandidateTiles(
    Tile* pTile,
    std::vector<std::string>& warnings) {

  // If tile failed to load, this means we can't complete the intersection
  if (pTile->getState() == TileLoadState::Failed) {
    warnings.push_back("Tile load failed during query. Ignoring.");
    return;
  }

  const std::optional<BoundingVolume>& contentBoundingVolume =
      pTile->getContentBoundingVolume();

  if (pTile->getChildren().empty()) {
    // This is a leaf node, it's a candidate

    // If optional content bounding volume exists, test against it
    if (contentBoundingVolume) {
      if (boundingVolumeContainsCoordinate(
              *contentBoundingVolume,
              this->ray,
              this->inputPosition))
        this->candidateTiles.push_back(pTile);
    } else {
      this->candidateTiles.push_back(pTile);
    }
  } else {
    // We have children

    // If additive refinement, add parent to the list with children
    if (pTile->getRefine() == TileRefine::Add) {
      // If optional content bounding volume exists, test against it
      if (contentBoundingVolume) {
        if (boundingVolumeContainsCoordinate(
                *contentBoundingVolume,
                this->ray,
                this->inputPosition))
          this->additiveCandidateTiles.push_back(pTile);
      } else {
        this->additiveCandidateTiles.push_back(pTile);
      }
    }

    // Traverse children
    for (Tile& child : pTile->getChildren()) {
      // if bounding volume doesn't intersect this ray, we can skip it
      if (!boundingVolumeContainsCoordinate(
              child.getBoundingVolume(),
              this->ray,
              this->inputPosition))
        continue;

      // Child is a candidate, traverse it and its children
      findCandidateTiles(&child, warnings);
    }
  }
}

/*static*/ void TilesetHeightRequest::processHeightRequests(
    TilesetContentManager& contentManager,
    const TilesetOptions& options,
    std::list<TilesetHeightRequest>& heightRequests,
    std::vector<Tile*>& heightQueryLoadQueue) {
  if (heightRequests.empty())
    return;

  // Go through all requests, either complete them, or gather the tiles they
  // need for completion
  std::set<Tile*> tilesNeedingLoading;
  for (auto it = heightRequests.begin(); it != heightRequests.end();) {
    TilesetHeightRequest& request = *it;
    if (!request.tryCompleteHeightRequest(
            contentManager,
            options,
            tilesNeedingLoading)) {
      ++it;
    } else {
      auto deleteIt = it;
      ++it;
      heightRequests.erase(deleteIt);
    }
  }

  heightQueryLoadQueue.assign(
      tilesNeedingLoading.begin(),
      tilesNeedingLoading.end());
}

bool TilesetHeightRequest::tryCompleteHeightRequest(
    TilesetContentManager& contentManager,
    const TilesetOptions& options,
    std::set<Tile*>& tilesNeedingLoading) {
  bool tileStillNeedsLoading = false;
  std::vector<std::string> warnings;
  for (TilesetHeightQuery& query : this->queries) {
    if (query.candidateTiles.empty() && query.additiveCandidateTiles.empty()) {
      // Find the initial set of tiles whose bounding volume is intersected by
      // the query ray.
      query.findCandidateTiles(contentManager.getRootTile(), warnings);
    } else {
      // Refine the current set of candidate tiles, in case further tiles from
      // implicit tiling, external tilesets, etc. having been loaded since last
      // frame.
      std::swap(query.candidateTiles, query.previousCandidateTiles);

      query.candidateTiles.clear();

      for (Tile* pCandidate : query.previousCandidateTiles) {
        TileLoadState loadState = pCandidate->getState();
        if (!pCandidate->getChildren().empty() &&
            loadState >= TileLoadState::ContentLoaded) {
          query.findCandidateTiles(pCandidate, warnings);
        } else {
          query.candidateTiles.emplace_back(pCandidate);
        }
      }
    }

    auto checkTile = [&contentManager,
                      &options,
                      &tilesNeedingLoading,
                      &tileStillNeedsLoading](Tile* pTile) {
      contentManager.createLatentChildrenIfNecessary(*pTile, options);

      TileLoadState state = pTile->getState();
      if (state == TileLoadState::Unloading) {
        // This tile is in the process of unloading, which must complete
        // before we can load it again.
        contentManager.unloadTileContent(*pTile);
        tileStillNeedsLoading = true;
      } else if (
          state == TileLoadState::Unloaded ||
          state == TileLoadState::FailedTemporarily) {
        tilesNeedingLoading.insert(pTile);
        tileStillNeedsLoading = true;
      }
    };

    // If any candidates need loading, add to return set
    for (Tile* pTile : query.additiveCandidateTiles) {
      checkTile(pTile);
    }
    for (Tile* pTile : query.candidateTiles) {
      checkTile(pTile);
    }
  }

  // Bail if we're waiting on tiles to load
  if (tileStillNeedsLoading)
    return false;

  // Do the intersect tests
  for (TilesetHeightQuery& query : this->queries) {
    for (Tile* pTile : query.additiveCandidateTiles) {
      query.intersectVisibleTile(pTile, warnings);
    }
    for (Tile* pTile : query.candidateTiles) {
      query.intersectVisibleTile(pTile, warnings);
    }
  }

  // All rays are done, create results
  SampleHeightResult results;

  // Start with any warnings from tile traversal
  results.warnings = std::move(warnings);

  results.positions.resize(this->queries.size(), Cartographic(0.0, 0.0, 0.0));
  results.heightSampled.resize(this->queries.size());

  // Populate results with completed queries
  for (size_t i = 0; i < this->queries.size(); ++i) {
    const TilesetHeightQuery& query = this->queries[i];

    bool heightSampled = query.intersection.has_value();
    results.heightSampled[i] = heightSampled;
    results.positions[i] = query.inputPosition;

    if (heightSampled) {
      results.positions[i].height =
          options.ellipsoid.getMaximumRadius() * rayOriginHeightFraction -
          glm::sqrt(query.intersection->rayToWorldPointDistanceSq);
    }
  }

  this->promise.resolve(std::move(results));
  return true;
}
