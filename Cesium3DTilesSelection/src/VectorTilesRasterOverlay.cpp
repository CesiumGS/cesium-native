#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/TileLoadRequester.h"
#include "Cesium3DTilesSelection/TileLoadTask.h"
#include "Cesium3DTilesSelection/TilesetSharedAssetSystem.h"
#include "Cesium3DTilesSelection/ViewState.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h"
#include "CesiumRasterOverlays/RasterOverlay.h"
#include "CesiumRasterOverlays/RasterOverlayTile.h"
#include "CesiumRasterOverlays/RasterOverlayTileProvider.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/VectorTilesRasterOverlay.h>
#include <mutex>
#include <queue>
#include <utility>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
struct SharedTileSelectionState
{
  std::vector<TileLoadTask> mainThreadLoadQueue;
  std::vector<TileLoadTask> workerThreadLoadQueue;
  std::vector<std::pair<CesiumGeospatial::GlobeRectangle, Tile::Pointer>> previouslySelectedTiles;

  std::mutex mutex;
};

class VectorTilesLoadRequester : public TileLoadRequester {
  virtual double getWeight() const override  { return this->_weight; }

  virtual bool hasMoreTilesToLoadInWorkerThread() const override {
    std::lock_guard<std::mutex> lock(this->_pSharedTileSelectionState->mutex);
    return !this->_pSharedTileSelectionState->workerThreadLoadQueue.empty();
  }

  const Tile* getNextTileToLoadInWorkerThread() override {
    std::lock_guard<std::mutex> lock(this->_pSharedTileSelectionState->mutex);
    CESIUM_ASSERT(!this->_pSharedTileSelectionState->workerThreadLoadQueue.empty());
    if (this->_pSharedTileSelectionState->workerThreadLoadQueue.empty())
      return nullptr;

    Tile* pResult = this->_pSharedTileSelectionState->workerThreadLoadQueue.back().pTile;
    this->_pSharedTileSelectionState->workerThreadLoadQueue.pop_back();
    return pResult;
  }

  bool hasMoreTilesToLoadInMainThread() const override {
    std::lock_guard<std::mutex> lock(this->_pSharedTileSelectionState->mutex);
    return !this->_pSharedTileSelectionState->mainThreadLoadQueue.empty();
  }

  const Tile* getNextTileToLoadInMainThread() override {
    std::lock_guard<std::mutex> lock(this->_pSharedTileSelectionState->mutex);
    CESIUM_ASSERT(!this->_pSharedTileSelectionState->mainThreadLoadQueue.empty());
    if (this->_pSharedTileSelectionState->mainThreadLoadQueue.empty())
      return nullptr;

    Tile* pResult = this->_pSharedTileSelectionState->mainThreadLoadQueue.back().pTile;
    this->_pSharedTileSelectionState->mainThreadLoadQueue.pop_back();
    return pResult;
  }

  VectorTilesLoadRequester(std::shared_ptr<SharedTileSelectionState> pSharedTileSelectionState)
    : _weight(1.0), _pSharedTileSelectionState(std::move(pSharedTileSelectionState)) {}

private:
  float _weight = 1.0;
  std::shared_ptr<SharedTileSelectionState> _pSharedTileSelectionState;
};

class VectorTilesRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {
private:
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;

  std::shared_ptr<SharedTileSelectionState> _pSharedTileSelectionState = std::make_shared<SharedTileSelectionState>();

public:
  VectorTilesRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const std::string& url)
      : RasterOverlayTileProvider(
            pCreator,
            parameters,
            CesiumGeospatial::GeographicProjection(
                CesiumGeospatial::Ellipsoid::WGS84),
            projectRectangleSimple(
                CesiumGeospatial::GeographicProjection(
                    CesiumGeospatial::Ellipsoid::WGS84),
                CesiumGeospatial::GlobeRectangle::MAXIMUM)){
    const TilesetExternals externals{
        parameters.externals.pAssetAccessor,
        nullptr,
        parameters.externals.asyncSystem,
        parameters.externals.pCreditSystem,
        parameters.externals.pLogger,
        nullptr,
        TilesetSharedAssetSystem::getDefault(),
        nullptr};

    this->_pTilesetContentManager =
        TilesetContentManager::createFromUrl(externals, TilesetOptions{}, url);
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::ivec2 textureSize = glm::min(
        glm::ivec2(
            overlayTile.getTargetScreenPixels() /
            options.maximumScreenSpaceError),
        glm::ivec2(options.maximumTextureSize));

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            overlayTile.getRectangle());
    const CesiumGeospatial::Ellipsoid& ellipsoid =
        CesiumGeospatial::Ellipsoid::WGS84;

    // Check if previously selected tiles cover this area
    std::lock_guard<std::mutex> lock(this->_pSharedTileSelectionState->mutex);
    bool isCovered = false;
    if (this->_pSharedTileSelectionState) {
      for (const auto& selectedTile : this->_pSharedTileSelectionState->previouslySelectedTiles) {
        if (selectedTile.first.computeIntersection(tileRectangle)) {
          // could be covered by multiple tiles? assume quadtree i guess...
          isCovered = true;
          break;
        }
      }
    }

    if (isCovered) {
      // This tile is already covered by previously selected tiles...
      // todo: check screenspace error
      // todo: actually render the tile
      return this->getAsyncSystem()
          .createResolvedFuture<LoadedRasterOverlayImage>(
              LoadedRasterOverlayImage{});
    }

    Tile* pCoveringTile = nullptr;
    Tile* pRoot = this->_pTilesetContentManager->getRootTile();
    
    if (pRoot) {
      std::queue<Tile*> tilesToCheck;
      tilesToCheck.push(pRoot);
      
      while (!tilesToCheck.empty() && !pCoveringTile) {
        Tile* pCurrentTile = tilesToCheck.front();
        tilesToCheck.pop();
        
        if (!pCurrentTile) { continue; }
        
        // Check if this tile covers the target rectangle
        const BoundingVolume& boundingVolume = pCurrentTile->getBoundingVolume();
        const std::optional<CesiumGeospatial::GlobeRectangle>& boundingRegion = estimateGlobeRectangle(boundingVolume);

        if (boundingRegion && boundingRegion->computeIntersection(tileRectangle)) {
          pCoveringTile = pCurrentTile;
          break;
        }

        if(pCurrentTile->getState() < TileLoadState::ContentLoaded) {
          // tile is not loaded, add to load queue and skip it for now
          this->_pSharedTileSelectionState->workerThreadLoadQueue.push_back(TileLoadTask{pCurrentTile, TileLoadPriorityGroup::Normal, 1.0});
          continue;
        }
        
        // Add children to the queue
        std::span<Tile> children = pCurrentTile->getChildren();
        for (Tile& child : children) {
          tilesToCheck.push(&child);
        }
      }
    }
    
    return this->getAsyncSystem()
        .createResolvedFuture<LoadedRasterOverlayImage>(
            LoadedRasterOverlayImage{});
  }
};
} // namespace

VectorTilesRasterOverlay::VectorTilesRasterOverlay(
    const std::string& name,
    const std::string& url,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions), _url(url) {}

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
VectorTilesRasterOverlay::createTileProvider(
    const CreateRasterOverlayTileProviderParameters& parameters) const {

  IntrusivePointer<const VectorTilesRasterOverlay> thiz = this;
  return parameters.externals.asyncSystem
      .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
          IntrusivePointer<RasterOverlayTileProvider>(
              new VectorTilesRasterOverlayTileProvider(
                  thiz,
                  parameters,
                  this->_url)));
}

} // namespace Cesium3DTilesSelection