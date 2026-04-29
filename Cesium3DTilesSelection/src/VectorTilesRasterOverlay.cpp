#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <Cesium3DTilesSelection/VectorTilesRasterOverlay.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/TreeTraversalState.h>

#include <concurrentqueue/moodycamel/concurrentqueue.h>

#include <cstdint>
#include <mutex>
#include <set>
#include <utility>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
struct LoadRequest {
  CesiumAsync::Promise<void> promise;
  std::set<uint64_t> requestedTileIds;
};

struct SharedTileSelectionState {
  moodycamel::ConcurrentQueue<std::pair<TileLoadTask, uint64_t>>
      workerThreadLoadQueue;
  // Does not need to be concurrent because it is only accessed by the
  // TilesetContentManager and the methods calling the TilesetContentManager.
  std::vector<std::pair<IntrusivePointer<Tile>, uint64_t>>
      tilesWaitingForWorker;
  std::mutex loadRequestsMutex;
  std::vector<LoadRequest> loadRequests;

  std::atomic<uint64_t> _nextId;
};

class VectorTilesLoadRequester : public TileLoadRequester {
  virtual double getWeight() const override { return this->_weight; }

  virtual bool hasMoreTilesToLoadInWorkerThread() const override {
    return this->_pSharedTileSelectionState->workerThreadLoadQueue
               .size_approx() > 0;
  }

  const Tile* getNextTileToLoadInWorkerThread() override {
    std::pair<TileLoadTask, uint64_t> taskAndId;
    if (!this->_pSharedTileSelectionState->workerThreadLoadQueue.try_dequeue(
            taskAndId)) {
      return nullptr;
    }

    this->_pSharedTileSelectionState->tilesWaitingForWorker.emplace_back(
        taskAndId.first.pTile,
        taskAndId.second);
    return taskAndId.first.pTile;
  }

  bool hasMoreTilesToLoadInMainThread() const override {
    // No main thread loading for vector tiles.
    return false;
  }

  const Tile* getNextTileToLoadInMainThread() override { return nullptr; }

  VectorTilesLoadRequester(
      std::shared_ptr<SharedTileSelectionState> pSharedTileSelectionState)
      : _weight(1.0),
        _pSharedTileSelectionState(std::move(pSharedTileSelectionState)) {}

private:
  float _weight = 1.0;
  std::shared_ptr<SharedTileSelectionState> _pSharedTileSelectionState;
};

class VectorTilesRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {
private:
  using TraversalState =
      CesiumUtility::TreeTraversalState<Tile::Pointer, TileSelectionState>;

  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
  TilesetOptions _options;

  std::shared_ptr<SharedTileSelectionState> _pSharedTileSelectionState =
      std::make_shared<SharedTileSelectionState>();

private:
  struct LoadTileImageInformation {
    CesiumGeospatial::GlobeRectangle tileRectangle;
    glm::ivec2 textureSize;
    std::vector<TileLoadTask> tileLoadTasks;
    std::vector<Tile::ConstPointer> tilesToRender;
  };

  CesiumAsync::Future<void> addLoadRequest(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::vector<TileLoadTask>& tasks) {
    std::set<uint64_t> requestedTileIds;
    for (const TileLoadTask& task : tasks) {
      uint64_t id = this->_pSharedTileSelectionState->_nextId++;
      this->_pSharedTileSelectionState->workerThreadLoadQueue.enqueue(
          {task, id});
      requestedTileIds.insert(id);
    }

    LoadRequest request{
        asyncSystem.createPromise<void>(),
        std::move(requestedTileIds)};
    CesiumAsync::Future<void> future = request.promise.getFuture();

    this->_pSharedTileSelectionState->loadRequestsMutex.lock();
    this->_pSharedTileSelectionState->loadRequests.emplace_back(
        std::move(request));
    this->_pSharedTileSelectionState->loadRequestsMutex.unlock();

    return future;
  }

  bool meetsSse(const Tile& tile, LoadTileImageInformation& loadInfo) {
    // TODO: is this SSE calculation actually correct?
    const double geometricError = tile.getGeometricError();
    const double pixelsPerMeter = (double)loadInfo.textureSize.y /
                                  (loadInfo.tileRectangle.computeHeight() *
                                   this->_options.ellipsoid.getRadii().x);

    return (geometricError * pixelsPerMeter) <
           this->_options.maximumScreenSpaceError;
  }

  void visit(Tile& tile, LoadTileImageInformation& loadInfo) {
    if (tile.isRenderContent() && meetsSse(tile, loadInfo)) {
      if (tile.getState() < TileLoadState::ContentLoaded) {
        loadInfo.tileLoadTasks.emplace_back(
            &tile,
            TileLoadPriorityGroup::Normal);
      }
      loadInfo.tilesToRender.emplace_back(&tile);
    } else {
      for (Tile& child : tile.getChildren()) {
        this->visitIfNeeded(child, loadInfo);
      }
    }
  }

  void visitIfNeeded(Tile& tile, LoadTileImageInformation& loadInfo) {
    this->_pTilesetContentManager->updateTileContent(tile, _options);

    const std::optional<CesiumGeospatial::GlobeRectangle> rectangle =
        estimateGlobeRectangle(
            tile.getBoundingVolume(),
            this->_options.ellipsoid);
    if (!rectangle) {
      // Invalid bounding rect, nothing to do with this.
      return;
    }

    if (rectangle->computeIntersection(loadInfo.tileRectangle)) {
      this->visit(tile, loadInfo);
    }
  }

  CesiumAsync::Future<LoadTileImageInformation> findAndLoadTiles(
      const CesiumGeospatial::GlobeRectangle& tileRectangle,
      const glm::ivec2& textureSize) {
    LoadTileImageInformation loadInfo{tileRectangle, textureSize, {}, {}};
    Tile* pRootTile = this->_pTilesetContentManager->getRootTile();

    if (pRootTile == nullptr) {
      return this->getAsyncSystem()
          .createResolvedFuture<LoadTileImageInformation>(std::move(loadInfo));
    }

    visitIfNeeded(*pRootTile, loadInfo);

    if (!loadInfo.tileLoadTasks.empty()) {
      // Add load request and re-run this function when all those tiles are
      // loaded. We need to re-run since some of those tiles might have been
      // external content and now we need to check their children.
      return this
          ->addLoadRequest(this->getAsyncSystem(), loadInfo.tileLoadTasks)
          .thenImmediately([this, tileRectangle, textureSize]() mutable {
            return this->findAndLoadTiles(tileRectangle, textureSize);
          });
    }

    return this->getAsyncSystem()
        .createResolvedFuture<LoadTileImageInformation>(std::move(loadInfo));
  }

  CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImageFromTileset(
      const CesiumGeometry::Rectangle& rectangle,
      const CesiumGeospatial::GlobeRectangle& tileRectangle,
      const glm::ivec2& textureSize) {
    return this->findAndLoadTiles(tileRectangle, textureSize)
        .thenInWorkerThread(
            [textureSize, rectangle](LoadTileImageInformation&& loadInfo) {
              // part 4 - rasterizing the tile
              LoadedRasterOverlayImage result;
              if (loadInfo.tilesToRender.empty()) {
                // No tiles to render, so return an empty image.
                result.rectangle = rectangle;
                result.moreDetailAvailable = false;
                result.pImage.emplace();
                result.pImage->width = 1;
                result.pImage->height = 1;
                result.pImage->channels = 4;
                result.pImage->bytesPerChannel = 1;
                result.pImage->pixelData = {
                    std::byte{0x00},
                    std::byte{0x00},
                    std::byte{0x00},
                    std::byte{0x00}};
              } else {
                result.moreDetailAvailable = true;
                result.pImage.emplace();
                result.pImage->width = textureSize.x;
                result.pImage->height = textureSize.y;
                result.pImage->channels = 4;
                result.pImage->bytesPerChannel = 1;
                result.pImage->pixelData.resize(
                    (size_t)(result.pImage->width * result.pImage->height *
                             result.pImage->channels *
                             result.pImage->bytesPerChannel),
                    std::byte{0});
                // TODO: rasterize here
              }

              return result;
            });
  }

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
                CesiumGeospatial::GlobeRectangle::MAXIMUM)),
        _options() {
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

    // part 1 - selecting from scratch
    if (this->_pTilesetContentManager->getRootTile() == nullptr) {
      return this->_pTilesetContentManager->getRootTileAvailableEvent()
          .thenInWorkerThread([this,
                               rectangle = overlayTile.getRectangle(),
                               tileRectangle,
                               textureSize]() mutable {
            return this->loadTileImageFromTileset(
                rectangle,
                tileRectangle,
                textureSize);
          });
    }

    return loadTileImageFromTileset(
        overlayTile.getRectangle(),
        tileRectangle,
        textureSize);
  }

  // TODO: how does this get called?
  virtual void tick() override {
    this->_pTilesetContentManager->processWorkerThreadLoadRequests(
        this->_options);

    // Check and resolve load requests if possible.
    this->_pSharedTileSelectionState->loadRequestsMutex.lock();
    for (const std::pair<IntrusivePointer<Tile>, uint64_t>& pair :
         this->_pSharedTileSelectionState->tilesWaitingForWorker) {
      if (pair.first->getState() < TileLoadState::ContentLoaded) {
        continue;
      }

      for (LoadRequest& request :
           this->_pSharedTileSelectionState->loadRequests) {
        if (request.requestedTileIds.erase(pair.second) > 0) {
          if (request.requestedTileIds.empty()) {
            request.promise.resolve();
          }
        }
      }
    }

    // Erase completed load requests.
    this->_pSharedTileSelectionState->loadRequests.erase(
        std::remove_if(
            this->_pSharedTileSelectionState->loadRequests.begin(),
            this->_pSharedTileSelectionState->loadRequests.end(),
            [](const LoadRequest& request) {
              return request.requestedTileIds.empty();
            }),
        this->_pSharedTileSelectionState->loadRequests.end());
    this->_pSharedTileSelectionState->loadRequestsMutex.unlock();

    // Erase tiles waiting for worker that are done.
    this->_pSharedTileSelectionState->tilesWaitingForWorker.erase(
        std::remove_if(
            this->_pSharedTileSelectionState->tilesWaitingForWorker.begin(),
            this->_pSharedTileSelectionState->tilesWaitingForWorker.end(),
            [](const std::pair<IntrusivePointer<Tile>, uint64_t>& pair) {
              return pair.first->getState() >= TileLoadState::ContentLoaded;
            }),
        this->_pSharedTileSelectionState->tilesWaitingForWorker.end());
  }

  virtual bool isTickable() override { return true; }
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