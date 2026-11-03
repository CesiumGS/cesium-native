#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3SContent/I3SToGltfConverter.h>
#include <CesiumI3SReader/NodePageReader.h>
#include <CesiumI3SReader/SceneLayerReader.h>
#include <CesiumI3SSelection/I3STilesetLoader.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Math.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;

namespace CesiumI3SSelection {

I3STilesetLoader::I3STilesetLoader(
    std::shared_ptr<IResourceLocator> pResourceLocator,
    CesiumI3S::Layer layer,
    std::shared_ptr<EarthGravitationalModel1996Grid> pGeoidGrid)
    : _pResourceLocator(std::move(pResourceLocator)),
      _layer(std::move(layer)),
      _pGeoidGrid(std::move(pGeoidGrid)) {}

uint32_t I3STilesetLoader::selectGeometryBufferIndex(
    uint32_t geometryDefinitionIndex) const noexcept {
  if (geometryDefinitionIndex >= this->_layer.geometryDefinitions.size()) {
    return 0;
  }
  const auto& geomDef =
      this->_layer.geometryDefinitions[geometryDefinitionIndex];
  for (uint32_t i = 0; i < geomDef.geometryBuffers.size(); ++i) {
    if (!geomDef.geometryBuffers[i].compression) {
      return i;
    }
  }
  return 0;
}

/**
 * Converts an I3S node's lodThreshold to a Cesium geometricError (metres).
 *
 * The metric type comes from the layer's nodePageDefinition and determines how
 * lodThreshold is interpreted:
 *
 *   MaxScreenThreshold / MaxScreenThresholdSQ
 *     — screen-space pixel threshold. Formula (matching CesiumJS):
 *         geometricError = (span / maxScreenDiameter) x 16
 *     where span = 2 x max(obb.halfSize) and maxScreenDiameter is derived
 *     from lodThreshold (converting SQ -> diameter for the SQ variant).
 *
 *   DistanceRangeFromDefaultCamera
 *     — lodThreshold is already in metres (distance from MBS surface to
 *     camera), so we use it directly as geometricError.
 *
 *   ScreenSpaceRelative / EffectiveDensity / DensityThreshold
 *     — point-cloud-specific metrics with no clean mapping to geometricError.
 *     An error is logged and 0.0 is returned so that Cesium treats the tile as
 *     a leaf node (show whatever geometry is present without further
 *     refinement) rather than causing unbounded tile loading.
 *
 * If lodThreshold is absent, a very large value (1.6e6 m) is returned so that
 * Cesium always refines through empty organisational nodes.
 */
static double geometricErrorFromNode(
    const CesiumI3S::Node& node,
    CesiumI3S::LodSelection::Metric metricType,
    spdlog::logger& logger) noexcept {
  if (!node.lodThreshold || *node.lodThreshold <= 0.0) {
    // No threshold defined — organisational/virtual node; always refine.
    return 1.6e6;
  }
  const double t = *node.lodThreshold;
  const double span =
      2.0 *
      std::max(
          {node.obb.halfSize[0], node.obb.halfSize[1], node.obb.halfSize[2]});

  using Metric = CesiumI3S::LodSelection::Metric;
  switch (metricType) {
  case Metric::MaxScreenThreshold:
    return (span / t) * 16.0;

  case Metric::MaxScreenThresholdSQ: {
    // lodThreshold = \PI x r^2  (projected MBS area in px^2)
    // -> screen diameter = 2 x sqrt(t / \PI) = sqrt(t / (\PI x 0.25))
    const double screenDiameter =
        std::sqrt(t / (CesiumUtility::Math::OnePi * 0.25));
    return (span / screenDiameter) * 16.0;
  }

  case Metric::DistanceRangeFromDefaultCamera:
    // lodThreshold is the camera-to-MBS-surface distance (metres) at which
    // this node is appropriate — use it directly as the geometric error.
    return t;

  case Metric::ScreenSpaceRelative:
  case Metric::EffectiveDensity:
  case Metric::DensityThreshold:
    SPDLOG_LOGGER_ERROR(
        &logger,
        "I3S node {}: unsupported lodSelectionMetricType '{}' — treating "
        "tile as a leaf node (geometricError=0). Point-cloud-specific LOD "
        "metrics are not supported for mesh layers.",
        node.index,
        static_cast<int>(metricType));
    return 0.0;
  }
  return 0.0;
}

BoundingVolume I3STilesetLoader::nodeObbToEcef(
    const CesiumI3S::OrientedBoundingBox& obb,
    const Ellipsoid& ellipsoid) const noexcept {
  // Step 1: convert OBB centre (lon/lat/height deg) to ECEF
  glm::dvec3 centerEcef = ellipsoid.cartographicToCartesian(
      Cartographic::fromDegrees(obb.center[0], obb.center[1], obb.center[2]));

  // Step 2: build ENU-to-ECEF rotation at the centre point
  glm::dvec3 up = glm::normalize(centerEcef);
  glm::dvec3 east = glm::normalize(glm::cross(glm::dvec3(0.0, 0.0, 1.0), up));
  // Handle degenerate case at poles
  if (glm::length(east) < 1e-10) {
    east = glm::dvec3(1.0, 0.0, 0.0);
  }
  glm::dvec3 north = glm::normalize(glm::cross(up, east));
  glm::dmat3 enuToEcef(east, north, up);

  // Step 3: convert I3S quaternion (x,y,z,w) to rotation matrix
  glm::dquat q(
      obb.quaternion[3],
      obb.quaternion[0],
      obb.quaternion[1],
      obb.quaternion[2]);
  glm::dmat3 obbRotation = glm::mat3_cast(q);

  // Step 4: compose half-axes matrix in ECEF
  glm::dmat3 scaleMatrix(
      glm::dvec3(obb.halfSize[0], 0.0, 0.0),
      glm::dvec3(0.0, obb.halfSize[1], 0.0),
      glm::dvec3(0.0, 0.0, obb.halfSize[2]));
  glm::dmat3 halfAxes = enuToEcef * obbRotation * scaleMatrix;

  return CesiumGeometry::OrientedBoundingBox(centerEcef, halfAxes);
}

std::shared_ptr<CesiumI3S::NodePage>
I3STilesetLoader::getNodePageFromCache(uint32_t pageIndex) const noexcept {
  std::lock_guard<std::mutex> lock(this->_nodePageCacheMutex);
  auto it = this->_nodePageCache.find(pageIndex);
  if (it == this->_nodePageCache.end()) {
    return nullptr;
  }
  // Return a copy of the shared_ptr so the caller holds a strong reference
  // even if the main thread evicts this entry from the cache concurrently.
  return it->second;
}

const CesiumI3S::Node*
I3STilesetLoader::getNodeFromCache(uint32_t nodeIndex) const noexcept {
  if (!this->_layer.nodePages) {
    return nullptr;
  }
  uint32_t nodesPerPage = this->_layer.nodePages->nodesPerPage;
  uint32_t pageIndex = nodeIndex / nodesPerPage;
  uint32_t offset = nodeIndex % nodesPerPage;

  // getNodeFromCache is only called from the main thread (createTileChildren,
  // createLoader). Eviction (onTileContentUnloaded) is also main-thread-only,
  // so the raw pointer is stable for the caller's synchronous use.
  std::shared_ptr<CesiumI3S::NodePage> pPage =
      this->getNodePageFromCache(pageIndex);
  if (!pPage || offset >= pPage->nodes.size()) {
    return nullptr;
  }
  return &pPage->nodes[offset];
}

Future<bool> I3STilesetLoader::fetchAndCacheNodePage(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    uint32_t pageIndex) {
  std::string url = this->_pResourceLocator->getNodePageUrl(pageIndex);
  return pAssetAccessor->get(asyncSystem, url, requestHeaders)
      .thenInWorkerThread(
          [this, pLogger, pageIndex](
              std::shared_ptr<IAssetRequest>&& pRequest) -> bool {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse || pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Failed to load I3S node page {}",
                  pageIndex);
              return false;
            }

            CesiumI3SReader::NodePageReader reader;
            auto result = reader.readNodePage(pResponse->data());
            if (!result.errors.empty()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Failed to parse I3S node page {}: {}",
                  pageIndex,
                  result.errors[0]);
              return false;
            }

            std::lock_guard<std::mutex> lock(this->_nodePageCacheMutex);
            this->_nodePageCache.emplace(
                pageIndex,
                std::make_shared<CesiumI3S::NodePage>(
                    std::move(*result.value)));
            return true;
          });
}

Future<bool> I3STilesetLoader::ensureNodePageCached(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    uint32_t nodeIndex) {
  if (!this->_layer.nodePages) {
    return asyncSystem.createResolvedFuture(false);
  }
  uint32_t pageIndex = nodeIndex / this->_layer.nodePages->nodesPerPage;
  {
    std::lock_guard<std::mutex> lock(this->_nodePageCacheMutex);
    if (this->_nodePageCache.find(pageIndex) != this->_nodePageCache.end()) {
      return asyncSystem.createResolvedFuture(true);
    }
  }
  return this->fetchAndCacheNodePage(
      asyncSystem,
      pAssetAccessor,
      pLogger,
      requestHeaders,
      pageIndex);
}

Future<bool> I3STilesetLoader::ensureChildPagesCached(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    const CesiumI3S::Node& node) {
  if (!this->_layer.nodePages || !node.children || node.children->empty()) {
    return asyncSystem.createResolvedFuture(true);
  }

  uint32_t nodesPerPage = this->_layer.nodePages->nodesPerPage;
  std::vector<uint32_t> pagesToFetch;
  {
    std::lock_guard<std::mutex> lock(this->_nodePageCacheMutex);
    for (uint32_t childIndex : *node.children) {
      uint32_t pageIndex = childIndex / nodesPerPage;
      if (this->_nodePageCache.find(pageIndex) == this->_nodePageCache.end() &&
          std::find(pagesToFetch.begin(), pagesToFetch.end(), pageIndex) ==
              pagesToFetch.end()) {
        pagesToFetch.push_back(pageIndex);
      }
    }
  }

  if (pagesToFetch.empty()) {
    return asyncSystem.createResolvedFuture(true);
  }

  std::vector<Future<bool>> futures;
  futures.reserve(pagesToFetch.size());
  for (uint32_t pageIndex : pagesToFetch) {
    futures.push_back(this->fetchAndCacheNodePage(
        asyncSystem,
        pAssetAccessor,
        pLogger,
        requestHeaders,
        pageIndex));
  }

  return asyncSystem.all(std::move(futures))
      .thenImmediately([](std::vector<bool>&& results) -> bool {
        for (bool ok : results) {
          if (!ok) {
            return false;
          }
        }
        return true;
      });
}

Future<TileLoadResult>
I3STilesetLoader::loadTileContent(const TileLoadInput& input) {
  // Copy async handles by value before any async boundary
  AsyncSystem asyncSystem = input.asyncSystem;
  std::shared_ptr<IAssetAccessor> pAssetAccessor = input.pAssetAccessor;
  std::shared_ptr<spdlog::logger> pLogger = input.pLogger;
  std::vector<IAssetAccessor::THeader> requestHeaders = input.requestHeaders;
  Ellipsoid ellipsoid = input.ellipsoid;

  const std::string* pIdStr = std::get_if<std::string>(&input.tile.getTileID());
  if (!pIdStr) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(nullptr, nullptr));
  }
  uint32_t nodeIndex = static_cast<uint32_t>(std::stoul(*pIdStr));

  return this
      ->ensureNodePageCached(
          asyncSystem,
          pAssetAccessor,
          pLogger,
          requestHeaders,
          nodeIndex)
      .thenInWorkerThread(
          [this,
           nodeIndex,
           asyncSystem,
           pAssetAccessor,
           pLogger,
           requestHeaders,
           ellipsoid](bool pageOk) mutable -> Future<TileLoadResult> {
            if (!pageOk) {
              return asyncSystem.createResolvedFuture(
                  TileLoadResult::createFailedResult(nullptr, nullptr));
            }

            uint32_t nodesPerPage = this->_layer.nodePages->nodesPerPage;
            uint32_t pageIndex = nodeIndex / nodesPerPage;
            uint32_t nodeOffset = nodeIndex % nodesPerPage;

            // Hold a shared_ptr so the page stays alive for the duration of
            // this callback even if the main thread evicts it concurrently.
            std::shared_ptr<CesiumI3S::NodePage> pPage =
                this->getNodePageFromCache(pageIndex);
            if (!pPage || nodeOffset >= pPage->nodes.size()) {
              return asyncSystem.createResolvedFuture(
                  TileLoadResult::createFailedResult(nullptr, nullptr));
            }
            const CesiumI3S::Node& node = pPage->nodes[nodeOffset];

            if (!node.mesh) {
              // No geometry — still pre-fetch child pages
              return this
                  ->ensureChildPagesCached(
                      asyncSystem,
                      pAssetAccessor,
                      pLogger,
                      requestHeaders,
                      node)
                  .thenImmediately(
                      [this, pageIndex](bool /*ok*/) -> TileLoadResult {
                        // Increment ref-count: this tile now holds the page
                        // alive. Decremented in onTileContentUnloaded when the
                        // tile is removed from the selection.
                        {
                          std::lock_guard<std::mutex> lock(
                              this->_nodePageCacheMutex);
                          ++this->_pageRefCounts[pageIndex];
                        }
                        TileLoadResult result;
                        result.contentKind = TileEmptyContent{};
                        result.state = TileLoadResultState::Success;
                        return result;
                      });
            }

            uint32_t bufferIndex =
                this->selectGeometryBufferIndex(node.mesh->geometry.definition);
            std::string geometryUrl = this->_pResourceLocator->getGeometryUrl(
                node.mesh->geometry.resource,
                bufferIndex);

            // Capture per-node data by value (node reference is in the page
            // which may remain valid, but we capture scalars for safety)
            glm::dvec3 center(
                node.obb.center[0],
                node.obb.center[1],
                node.obb.center[2]);
            glm::dquat rotation(
                node.obb.quaternion[3],
                node.obb.quaternion[0],
                node.obb.quaternion[1],
                node.obb.quaternion[2]);
            uint32_t geomDefinitionIndex = node.mesh->geometry.definition;
            std::optional<uint32_t> materialDefinitionIndex =
                node.mesh->material
                    ? std::optional<uint32_t>(node.mesh->material->definition)
                    : std::nullopt;

            auto geomFuture =
                pAssetAccessor->get(asyncSystem, geometryUrl, requestHeaders);
            auto childPageFuture = this->ensureChildPagesCached(
                asyncSystem,
                pAssetAccessor,
                pLogger,
                requestHeaders,
                node);

            return asyncSystem
                .all(std::move(geomFuture), std::move(childPageFuture))
                .thenInWorkerThread(
                    [this,
                     pageIndex,
                     bufferIndex,
                     geomDefinitionIndex,
                     materialDefinitionIndex,
                     center,
                     rotation,
                     ellipsoid,
                     pAssetAccessor](
                        std::tuple<std::shared_ptr<IAssetRequest>, bool>&&
                            results) -> TileLoadResult {
                      auto pGeomRequest = std::move(std::get<0>(results));
                      const IAssetResponse* pResponse =
                          pGeomRequest->response();
                      if (!pResponse || pResponse->statusCode() < 200 ||
                          pResponse->statusCode() >= 300) {
                        return TileLoadResult::createFailedResult(
                            pAssetAccessor,
                            std::move(pGeomRequest));
                      }

                      const CesiumI3S::GeometryBuffer* pGeomBuffer = nullptr;
                      if (geomDefinitionIndex <
                          this->_layer.geometryDefinitions.size()) {
                        const auto& geomDef =
                            this->_layer
                                .geometryDefinitions[geomDefinitionIndex];
                        if (bufferIndex < geomDef.geometryBuffers.size()) {
                          pGeomBuffer = &geomDef.geometryBuffers[bufferIndex];
                        }
                      }
                      if (!pGeomBuffer) {
                        return TileLoadResult::createFailedResult(
                            pAssetAccessor,
                            std::move(pGeomRequest));
                      }

                      const CesiumI3S::MaterialDefinition* pMat = nullptr;
                      if (materialDefinitionIndex &&
                          *materialDefinitionIndex <
                              this->_layer.materialDefinitions.size()) {
                        pMat =
                            &this->_layer
                                 .materialDefinitions[*materialDefinitionIndex];
                      }

                      CesiumI3SContent::I3SToGltfConverterInput converterInput;
                      converterInput.geometryBytes = pResponse->data();
                      converterInput.pGeometryBuffer = pGeomBuffer;
                      converterInput.nodeCenterLonLatDegHeight = center;
                      converterInput.nodeObbRotation = rotation;
                      converterInput.pMaterialDef = pMat;
                      converterInput.pGeoidGrid = this->_pGeoidGrid.get();
                      if (this->_layer.spatialReference) {
                        converterInput.spatialReference =
                            *this->_layer.spatialReference;
                      }
                      converterInput.heightModelInfo =
                          this->_layer.heightModelInfo;

                      CesiumI3SContent::I3SConverterResult converted =
                          CesiumI3SContent::I3SToGltfConverter::convert(
                              converterInput);
                      if (!converted.model) {
                        return TileLoadResult::createFailedResult(
                            pAssetAccessor,
                            std::move(pGeomRequest));
                      }

                      TileLoadResult result;
                      result.contentKind = std::move(*converted.model);
                      result.glTFUpAxis = CesiumGeometry::Axis::Y;
                      result.state = TileLoadResultState::Success;
                      result.ellipsoid = ellipsoid;
                      result.pAssetAccessor = pAssetAccessor;
                      result.pCompletedRequest = std::move(pGeomRequest);
                      // Increment ref-count: this tile now holds the page
                      // alive.
                      {
                        std::lock_guard<std::mutex> lock(
                            this->_nodePageCacheMutex);
                        ++this->_pageRefCounts[pageIndex];
                      }
                      return result;
                    });
          });
}

TileChildrenResult I3STilesetLoader::createTileChildren(
    const Tile& tile,
    const Ellipsoid& ellipsoid) {
  // loadTileContent pre-fetches the child node pages as a side-effect.
  // Until loadTileContent has completed (tile is Done), those pages are not
  // yet in the cache.  Returning RetryLater here avoids a busy-spin: the
  // framework will call us again on the next update after the tile finishes
  // loading, at which point the page cache is guaranteed to be populated.
  if (tile.getState() != TileLoadState::Done) {
    return {{}, TileLoadResultState::RetryLater};
  }

  const std::string* pIdStr = std::get_if<std::string>(&tile.getTileID());
  if (!pIdStr) {
    return {{}, TileLoadResultState::Failed};
  }
  uint32_t nodeIndex = static_cast<uint32_t>(std::stoul(*pIdStr));

  const CesiumI3S::Node* pNode = this->getNodeFromCache(nodeIndex);
  if (!pNode) {
    return {{}, TileLoadResultState::Failed};
  }
  if (!pNode->children || pNode->children->empty()) {
    return {{}, TileLoadResultState::Success};
  }

  TileChildrenResult result;
  result.children.reserve(pNode->children->size());
  for (uint32_t childIndex : *pNode->children) {
    const CesiumI3S::Node* pChild = this->getNodeFromCache(childIndex);
    if (!pChild) {
      // This should not happen: loadTileContent pre-fetches all child pages
      // before the tile reaches Done state. Log and skip gracefully.
      SPDLOG_WARN(
          "I3STilesetLoader::createTileChildren: child node {} of node {} not "
          "in cache despite parent being Done — skipping",
          childIndex,
          nodeIndex);
      continue;
    }

    Tile& child =
        result.children.emplace_back(this, std::to_string(childIndex));
    child.setBoundingVolume(this->nodeObbToEcef(pChild->obb, ellipsoid));
    if (!pChild->mesh) {
      // No renderable geometry — this is an organisational node that must
      // always be refined through.  setUnconditionallyRefine() sets
      // geometricError = infinity so the selection algorithm never treats it
      // as a leaf, regardless of the parent/child error comparison.
      child.setUnconditionallyRefine();
    } else {
      const double geometricError = geometricErrorFromNode(
          *pChild,
          this->_layer.nodePages->lodSelectionMetricType,
          *spdlog::default_logger());
      child.setGeometricError(geometricError);
    }
    child.setRefine(TileRefine::Replace);
  }
  result.state = TileLoadResultState::Success;
  return result;
}

Future<TilesetContentLoaderResult<I3STilesetLoader>>
I3STilesetLoader::createLoader(
    const TilesetExternals& externals,
    std::shared_ptr<IResourceLocator> pResourceLocator,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    const std::shared_ptr<EarthGravitationalModel1996Grid>& pGeoidGrid) {
  AsyncSystem asyncSystem = externals.asyncSystem;
  std::shared_ptr<IAssetAccessor> pAssetAccessor = externals.pAssetAccessor;
  std::string layerUrl = pResourceLocator->getLayerUrl();

  return pAssetAccessor->get(asyncSystem, layerUrl, requestHeaders)
      .thenInWorkerThread(
          [asyncSystem,
           pAssetAccessor,
           pResourceLocator = std::move(pResourceLocator),
           layerUrl,
           requestHeaders,
           pGeoidGrid](std::shared_ptr<IAssetRequest>&& pRequest) mutable
          -> Future<TilesetContentLoaderResult<I3STilesetLoader>> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse || pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              TilesetContentLoaderResult<I3STilesetLoader> errorResult;
              errorResult.errors.emplaceError(
                  "Failed to load I3S layer JSON from: " + layerUrl);
              return asyncSystem.createResolvedFuture(std::move(errorResult));
            }

            CesiumI3SReader::SceneLayerReader reader;
            auto layerResult = reader.readLayer(pResponse->data());
            if (!layerResult.errors.empty() || !layerResult.value) {
              TilesetContentLoaderResult<I3STilesetLoader> errorResult;
              if (!layerResult.errors.empty()) {
                errorResult.errors.emplaceError(
                    "Failed to parse I3S layer JSON: " + layerResult.errors[0]);
              } else {
                errorResult.errors.emplaceError(
                    "Failed to parse I3S layer JSON");
              }
              return asyncSystem.createResolvedFuture(std::move(errorResult));
            }

            CesiumI3S::Layer layer = std::move(*layerResult.value);
            if (!layer.nodePages) {
              TilesetContentLoaderResult<I3STilesetLoader> errorResult;
              errorResult.errors.emplaceError(
                  "I3S layer has no nodePages definition — only 1.7+ node-page "
                  "layers are supported");
              return asyncSystem.createResolvedFuture(std::move(errorResult));
            }

            uint32_t rootIndex = layer.nodePages->rootIndex;
            uint32_t nodesPerPage = layer.nodePages->nodesPerPage;
            uint32_t rootPageIndex = rootIndex / nodesPerPage;

            auto pLoader =
                std::unique_ptr<I3STilesetLoader>(new I3STilesetLoader(
                    std::move(pResourceLocator),
                    std::move(layer),
                    pGeoidGrid));

            return pLoader
                ->fetchAndCacheNodePage(
                    asyncSystem,
                    pAssetAccessor,
                    spdlog::default_logger(),
                    requestHeaders,
                    rootPageIndex)
                .thenImmediately(
                    [pLoader = std::move(pLoader),
                     rootIndex,
                     pAssetAccessor,
                     requestHeaders](bool pageOk) mutable
                    -> TilesetContentLoaderResult<I3STilesetLoader> {
                      if (!pageOk) {
                        TilesetContentLoaderResult<I3STilesetLoader>
                            errorResult;
                        errorResult.errors.emplaceError(
                            "Failed to load root I3S node page");
                        return errorResult;
                      }

                      const CesiumI3S::Node* pRoot =
                          pLoader->getNodeFromCache(rootIndex);
                      if (!pRoot) {
                        TilesetContentLoaderResult<I3STilesetLoader>
                            errorResult;
                        errorResult.errors.emplaceError(
                            "Root I3S node not found in page cache");
                        return errorResult;
                      }

                      auto pRootTile = std::make_unique<Tile>(
                          pLoader.get(),
                          std::to_string(rootIndex));
                      pRootTile->setBoundingVolume(
                          pLoader->nodeObbToEcef(pRoot->obb, Ellipsoid::WGS84));
                      if (!pRoot->mesh) {
                        // Root has no renderable geometry — always refine.
                        pRootTile->setUnconditionallyRefine();
                      } else {
                        const double geometricError = geometricErrorFromNode(
                            *pRoot,
                            pLoader->_layer.nodePages->lodSelectionMetricType,
                            *spdlog::default_logger());
                        pRootTile->setGeometricError(geometricError);
                      }
                      pRootTile->setRefine(TileRefine::Replace);

                      std::vector<IAssetAccessor::THeader> hdrs(requestHeaders);
                      return TilesetContentLoaderResult<I3STilesetLoader>(
                          std::move(pLoader),
                          std::move(pRootTile),
                          {},
                          std::move(hdrs),
                          {});
                    });
          });
}

void I3STilesetLoader::onTileContentUnloaded(const Tile& tile) noexcept {
  if (!this->_layer.nodePages) {
    return;
  }
  const std::string* pIdStr = std::get_if<std::string>(&tile.getTileID());
  if (!pIdStr) {
    return;
  }

  uint32_t nodeIndex;
  try {
    nodeIndex = static_cast<uint32_t>(std::stoul(*pIdStr));
  } catch (...) {
    return;
  }

  const uint32_t pageIndex = nodeIndex / this->_layer.nodePages->nodesPerPage;

  std::lock_guard<std::mutex> lock(this->_nodePageCacheMutex);
  auto it = this->_pageRefCounts.find(pageIndex);
  if (it == this->_pageRefCounts.end()) {
    // This tile's load either failed or never incremented the ref-count
    // (e.g. a pre-fetched page that was never claimed).
    return;
  }
  if (--it->second == 0u) {
    this->_pageRefCounts.erase(it);
    this->_nodePageCache.erase(pageIndex);
  }
}

} // namespace CesiumI3SSelection
