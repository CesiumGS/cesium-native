#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/TileLoadResult.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGltf/AccessorUtility.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltfContent/GltfUtilities.h"
#include "CesiumUtility/ErrorList.h"
#include "CesiumVectorData/VectorRasterizer.h"
#include "CesiumVectorData/VectorStyle.h"
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

#include <fmt/format.h>

#include <cstdint>
#include <mutex>
#include <set>
#include <utility>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
struct VectorRenderContent {
  CesiumVectorData::VectorStyle style;

  std::vector<CesiumGeospatial::Cartographic> points;
  std::vector<std::vector<CesiumGeospatial::Cartographic>> polylines;
  std::vector<std::vector<std::vector<CesiumGeospatial::Cartographic>>>
      polygons;
};

CesiumUtility::Result<VectorRenderContent*> vectorizeModel(
    CesiumGltf::Model& model,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dmat4x4& gltfTransform,
    const CesiumVectorData::VectorStyle& defaultStyle) {

  glm::dmat4x4 rootTransform =
      CesiumGltfContent::GltfUtilities::applyRtcCenter(model, gltfTransform);
  rootTransform = CesiumGltfContent::GltfUtilities::applyGltfUpAxisTransform(
      model,
      rootTransform);

  CesiumUtility::ErrorList errors;

  VectorRenderContent* content = new VectorRenderContent();
  content->style = defaultStyle;

  model.forEachPrimitiveInScene(
      -1,
      [content, rootTransform, &errors, &ellipsoid](
          const CesiumGltf::Model& model,
          const CesiumGltf::Node& /*node*/,
          const CesiumGltf::Mesh& /*mesh*/,
          CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& nodeTransform) mutable {
        const CesiumGltf::AccessorView<glm::vec3> positionView(
            model,
            primitive.attributes.at("POSITION"));
        const CesiumGltf::IndexAccessorType indicesView =
            CesiumGltf::getIndexAccessorView(model, primitive);
        const int64_t numIndices =
            std::visit(CesiumGltf::NumIndicesFromAccessor{}, indicesView);
        const int64_t maxIndex =
            std::visit(CesiumGltf::MaxIndexValueFromAccessor{}, indicesView);

        if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
          errors.emplaceError(
              "Cannot create valid position accessor for primitive.");
          return;
        }

        if (primitive.mode == CesiumGltf::MeshPrimitive::Mode::POINTS) {
          for (int64_t i = 0; i < positionView.size(); i++) {
            const glm::vec3 position = positionView[i];
            const glm::dvec4 transformedPosition =
                rootTransform * nodeTransform * glm::dvec4(position, 1.0);
            content->points.emplace_back(
                ellipsoid
                    .cartesianToCartographic(glm::dvec3(transformedPosition))
                    .value_or(CesiumGeospatial::Cartographic{0.0, 0.0, 0.0}));
          }
          return;
        }

        if (primitive.mode == CesiumGltf::MeshPrimitive::Mode::LINE_STRIP) {
          if (numIndices < 2) {
            errors.emplaceError(
                "Primitive mode LINE_STRIP requires at least two indices.");
            return;
          }

          std::vector<CesiumGeospatial::Cartographic> polyline;
          for (int64_t i = 0; i < numIndices; i++) {
            const int64_t idx =
                std::visit(CesiumGltf::IndexFromAccessor{i}, indicesView);
            if (idx == maxIndex) {
              // Primitive restart.
              if (polyline.size() >= 2) {
                content->polylines.emplace_back(std::move(polyline));
                polyline.clear();
              } else {
                errors.emplaceError("Primitive restart index encountered but "
                                    "current polyline has less than 2 points.");
              }

              continue;
            }

            if (idx < 0 || idx >= positionView.size()) {
              errors.emplaceError(fmt::format(
                  "Index {} out of bounds for position accessor size {}",
                  idx,
                  positionView.size()));
              return;
            }

            const glm::vec3 position = positionView[idx];
            const glm::dvec4 transformedPosition =
                rootTransform * nodeTransform * glm::dvec4(position, 1.0);
            polyline.emplace_back(
                ellipsoid
                    .cartesianToCartographic(glm::dvec3(transformedPosition))
                    .value_or(CesiumGeospatial::Cartographic{0.0, 0.0, 0.0}));
          }

          if (polyline.size() >= 2) {
            content->polylines.emplace_back(std::move(polyline));
          } else if (polyline.size() == 1) {
            errors.emplaceWarning("LINE_STRIP primitive ended with a single "
                                  "point unassigned to a line.");
          }
        }

        if (primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES) {
          // TODO: figure out what's going on with that polygon extension
        }
      });

  return {content, errors};
}

struct LoadRequest {
  CesiumAsync::Promise<void> promise;
  std::set<Tile::ConstPointer> requestedTiles;
};

struct SharedTileSelectionState {
  std::vector<TileLoadTask> workerThreadLoadQueue;
  // Does not need to be concurrent because it is only accessed by the
  // TilesetContentManager and the methods calling the TilesetContentManager.
  std::vector<Tile::Pointer> tilesWaitingForWorker;
  std::mutex loadRequestsMutex;
  std::vector<LoadRequest> loadRequests;
};

class VectorTilesLoadRequester : public TileLoadRequester {
  virtual double getWeight() const override { return this->_weight; }

  virtual bool hasMoreTilesToLoadInWorkerThread() const override {
    return !this->_pSharedTileSelectionState->workerThreadLoadQueue.empty();
  }

  const Tile* getNextTileToLoadInWorkerThread() override {
    if (this->_pSharedTileSelectionState->workerThreadLoadQueue.empty()) {
      return nullptr;
    }

    TileLoadTask& task =
        this->_pSharedTileSelectionState->workerThreadLoadQueue.back();

    this->_pSharedTileSelectionState->tilesWaitingForWorker.emplace_back(
        task.pTile);
    const Tile::Pointer pTile = task.pTile;
    this->_pSharedTileSelectionState->workerThreadLoadQueue.pop_back();
    return pTile;
  }

  bool hasMoreTilesToLoadInMainThread() const override {
    // No main thread loading for vector tiles.
    return false;
  }

  const Tile* getNextTileToLoadInMainThread() override { return nullptr; }

public:
  VectorTilesLoadRequester(
      std::shared_ptr<SharedTileSelectionState> pSharedTileSelectionState)
      : _weight(1.0),
        _pSharedTileSelectionState(std::move(pSharedTileSelectionState)) {}

private:
  float _weight = 1.0;
  std::shared_ptr<SharedTileSelectionState> _pSharedTileSelectionState;
};

class VectorTilesPrepareRendererResources : public IPrepareRendererResources {
public:
  virtual CesiumAsync::Future<TileLoadResultAndRenderResources>
  prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TileLoadResult&& tileLoadResult,
      const glm::dmat4& transform,
      const std::any& /*rendererOptions*/) override {
    CesiumGltf::Model* pModel =
        std::get_if<CesiumGltf::Model>(&tileLoadResult.contentKind);
    if (pModel == nullptr) {
      return asyncSystem.createResolvedFuture(
          TileLoadResultAndRenderResources{std::move(tileLoadResult), nullptr});
    }

    Result<VectorRenderContent*> vectorizationResult = vectorizeModel(
        *pModel,
        tileLoadResult.ellipsoid,
        transform,
        this->defaultStyle);
    if (vectorizationResult.errors.hasErrors()) {
      if (vectorizationResult.value) {
        delete *vectorizationResult.value;
      }

      vectorizationResult.errors.log(
          this->pLogger,
          "Errors when vectorizing model:");
      return asyncSystem.createResolvedFuture(
          TileLoadResultAndRenderResources{std::move(tileLoadResult), nullptr});
    }

    return asyncSystem.createResolvedFuture(TileLoadResultAndRenderResources{
        std::move(tileLoadResult),
        vectorizationResult.value ? *vectorizationResult.value : nullptr});
  }

  void* prepareInMainThread(Tile& /*tile*/, void* pLoadThreadResult) override {
    // No main thread preparation for vector tiles.
    return pLoadThreadResult;
  }

  void free(
      Tile& /*tile*/,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pLoadThreadResult != nullptr) {
      VectorRenderContent* pContent =
          static_cast<VectorRenderContent*>(pLoadThreadResult);
      delete pContent;
    }

    if (pMainThreadResult != nullptr) {
      VectorRenderContent* pContent =
          static_cast<VectorRenderContent*>(pMainThreadResult);
      delete pContent;
    }
  }

  void attachRasterInMainThread(
      const Tile& /*tile*/,
      int32_t /*overlayTextureCoordinateID*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/,
      const glm::dvec2& /*translation*/,
      const glm::dvec2& /*scale*/) override {}

  void detachRasterInMainThread(
      const Tile& /*tile*/,
      int32_t /*overlayTextureCoordinateID*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/) noexcept override {}

  virtual void* prepareRasterInLoadThread(
      CesiumGltf::ImageAsset& /*image*/,
      const std::any& /*rendererOptions*/) override {
    return nullptr;
  }

  virtual void* prepareRasterInMainThread(
      RasterOverlayTile& /*rasterTile*/,
      void* /*pLoadThreadResult*/) override {
    return nullptr;
  }

  virtual void freeRaster(
      const RasterOverlayTile& /*rasterTile*/,
      void* /*pLoadThreadResult*/,
      void* /*pMainThreadResult*/) noexcept override {}

  VectorTilesPrepareRendererResources(
      std::shared_ptr<spdlog::logger> pLogger,
      CesiumVectorData::VectorStyle defaultStyle)
      : defaultStyle(std::move(defaultStyle)), pLogger(std::move(pLogger)) {}

  CesiumVectorData::VectorStyle defaultStyle;
  std::shared_ptr<spdlog::logger> pLogger;
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
  VectorTilesLoadRequester _loadRequester{_pSharedTileSelectionState};
  std::shared_ptr<VectorTilesPrepareRendererResources>
      _pPrepareRendererResources;

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
    std::set<Tile::ConstPointer> requestedTiles;
    for (const TileLoadTask& task : tasks) {
      this->_pSharedTileSelectionState->workerThreadLoadQueue.emplace_back(
          task);
      requestedTiles.insert(task.pTile);
    }

    LoadRequest request{
        asyncSystem.createPromise<void>(),
        std::move(requestedTiles)};
    CesiumAsync::Future<void> future = request.promise.getFuture();

    std::scoped_lock<std::mutex> lock(
        this->_pSharedTileSelectionState->loadRequestsMutex);
    this->_pSharedTileSelectionState->loadRequests.emplace_back(
        std::move(request));

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
    if (tile.getState() < TileLoadState::ContentLoaded) {
      loadInfo.tileLoadTasks.emplace_back(&tile, TileLoadPriorityGroup::Normal);
      return;
    }

    if (tile.isRenderContent() &&
        (meetsSse(tile, loadInfo) || tile.getChildren().empty())) {
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
          .thenInWorkerThread([this, tileRectangle, textureSize]() mutable {
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
        .thenInWorkerThread([textureSize,
                             rectangle,
                             tileRectangle,
                             ellipsoid = this->_options.ellipsoid](
                                LoadTileImageInformation&& loadInfo) {
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
            CesiumVectorData::VectorRasterizer rasterizer(
                tileRectangle,
                result.pImage,
                0,
                ellipsoid);

            for (const Tile::ConstPointer& pTile : loadInfo.tilesToRender) {
              const TileRenderContent* pRenderContent =
                  pTile->getContent().getRenderContent();
              if (pRenderContent == nullptr) {
                continue;
              }

              const VectorRenderContent* pVectorContent =
                  static_cast<const VectorRenderContent*>(
                      pRenderContent->getRenderResources());
              if (pVectorContent == nullptr) {
                continue;
              }

              for (const std::vector<CesiumGeospatial::Cartographic>& polyline :
                   pVectorContent->polylines) {
                rasterizer.drawPolyline(polyline, pVectorContent->style.line);
              }

              if (!pVectorContent->points.empty()) {
                rasterizer.drawPoints(
                    pVectorContent->points,
                    pVectorContent->style.point);
              }
            }

            rasterizer.finalize();
          }

          return result;
        });
  }

public:
  VectorTilesRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const std::string& url,
      const CesiumVectorData::VectorStyle& defaultStyle)
      : RasterOverlayTileProvider(
            pCreator,
            parameters,
            CesiumGeospatial::GeographicProjection(
                CesiumGeospatial::Ellipsoid::WGS84),
            projectRectangleSimple(
                CesiumGeospatial::GeographicProjection(
                    CesiumGeospatial::Ellipsoid::WGS84),
                CesiumGeospatial::GlobeRectangle::MAXIMUM)),
        _options(),
        _pPrepareRendererResources(
            std::make_shared<VectorTilesPrepareRendererResources>(
                parameters.externals.pLogger,
                defaultStyle)) {
    const TilesetExternals externals{
        parameters.externals.pAssetAccessor,
        this->_pPrepareRendererResources,
        parameters.externals.asyncSystem,
        parameters.externals.pCreditSystem,
        parameters.externals.pLogger,
        nullptr,
        TilesetSharedAssetSystem::getDefault(),
        nullptr};

    this->_pTilesetContentManager =
        TilesetContentManager::createFromUrl(externals, TilesetOptions{}, url);
    this->_pTilesetContentManager->registerTileRequester(this->_loadRequester);
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
    this->_pTilesetContentManager->processMainThreadLoadRequests(
        this->_options);

    // Check and resolve load requests if possible.
    std::vector<CesiumAsync::Promise<void>> promisesToResolve;
    {
      std::scoped_lock<std::mutex> lock(
          this->_pSharedTileSelectionState->loadRequestsMutex);
      for (const Tile::Pointer& pTile :
           this->_pSharedTileSelectionState->tilesWaitingForWorker) {
        TileLoadState state = pTile->getState();
        if (state == TileLoadState::FailedTemporarily) {
          this->_pSharedTileSelectionState->workerThreadLoadQueue.emplace_back(
              pTile.get(),
              TileLoadPriorityGroup::Urgent,
              1.0);
        } else if (state < TileLoadState::ContentLoaded) {
          continue;
        }

        for (LoadRequest& request :
             this->_pSharedTileSelectionState->loadRequests) {
          if (request.requestedTiles.erase(pTile) > 0) {
            if (request.requestedTiles.empty()) {
              // Keep list of promises to resolve until after we've unlocked the
              // mutex.
              promisesToResolve.emplace_back(std::move(request.promise));
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
                return request.requestedTiles.empty();
              }),
          this->_pSharedTileSelectionState->loadRequests.end());
    }

    for (CesiumAsync::Promise<void>& promise : promisesToResolve) {
      promise.resolve();
    }

    // Erase tiles waiting for worker that are done.
    this->_pSharedTileSelectionState->tilesWaitingForWorker.erase(
        std::remove_if(
            this->_pSharedTileSelectionState->tilesWaitingForWorker.begin(),
            this->_pSharedTileSelectionState->tilesWaitingForWorker.end(),
            [](const Tile::Pointer& pTile) {
              return pTile->getState() >= TileLoadState::ContentLoaded ||
                     pTile->getState() == TileLoadState::FailedTemporarily;
            }),
        this->_pSharedTileSelectionState->tilesWaitingForWorker.end());
  }

  virtual bool isTickable() override { return true; }
};
} // namespace

VectorTilesRasterOverlay::VectorTilesRasterOverlay(
    const std::string& name,
    const std::string& url,
    const CesiumVectorData::VectorStyle& defaultStyle,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions),
      _url(url),
      _defaultStyle(defaultStyle) {}

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
                  this->_url,
                  this->_defaultStyle)));
}

} // namespace Cesium3DTilesSelection