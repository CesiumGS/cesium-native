// Integration test — requires network access and libcurl.
// Guarded entirely so it compiles away when CesiumCurl is disabled.
#ifndef CESIUM_DISABLE_CURL

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Model.h>
#include <CesiumI3SSelection/I3STilesetLoaderFactory.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <any>
#include <cmath>
#include <memory>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {

class NullPrepareRendererResources final
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  CesiumAsync::Future<TileLoadResultAndRenderResources> prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TileLoadResult&& tileLoadResult,
      const glm::dmat4& /*transform*/,
      const std::any& /*rendererOptions*/) override {
    return asyncSystem.createResolvedFuture(
        TileLoadResultAndRenderResources{std::move(tileLoadResult), nullptr});
  }

  void* prepareInMainThread(Tile& /*tile*/, void* /*pLoad*/) override {
    return nullptr;
  }

  void
  free(Tile& /*tile*/, void* /*pLoad*/, void* /*pMain*/) noexcept override {}

  void attachRasterInMainThread(
      const Tile& /*tile*/,
      int32_t /*overlayTexCoordId*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/,
      const glm::dvec2& /*translation*/,
      const glm::dvec2& /*scale*/) override {}

  void detachRasterInMainThread(
      const Tile& /*tile*/,
      int32_t /*overlayTexCoordId*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/) noexcept override {}

  void* prepareRasterInLoadThread(
      CesiumGltf::ImageAsset& /*image*/,
      const std::any& /*rendererOptions*/) override {
    return nullptr;
  }

  void* prepareRasterInMainThread(
      CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pLoadThreadResult*/) override {
    return nullptr;
  }

  void freeRaster(
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pLoadThreadResult*/,
      void* /*pMainThreadResult*/) noexcept override {}
};

} // anonymous namespace

// Public I3S dataset — Esri ArcGIS Online sample, no authentication required.
static constexpr const char* kLayerUrl =
    "https://tiles.arcgis.com/tiles/z2tnIkrLQ2BRzr6P/arcgis/rest/services/"
    "SanFrancisco_Bldgs/SceneServer/layers/0";

TEST_CASE("I3S HTTP: gltf content is produced from a camera position") {
  auto pAssetAccessor = std::make_shared<CesiumCurl::CurlAssetAccessor>();
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::ThreadTaskProcessor>());

  TilesetExternals externals{
      pAssetAccessor,
      std::make_shared<NullPrepareRendererResources>(),
      asyncSystem,
      nullptr};

  // Small SSE so non-root tiles need to load.
  TilesetOptions options;
  Tileset tileset(
      externals,
      CesiumI3SSelection::I3STilesetLoaderFactory(kLayerUrl),
      options);

  // Wait for root tile so we can read its bounding volume.
  tileset.getRootTileAvailableEvent().waitInMainThread();
  const Tile* pRootTile = tileset.getRootTile();
  REQUIRE(pRootTile != nullptr);

  // Derive camera geometry from the root OBB center.
  const glm::dvec3 obbCenter = Cesium3DTilesSelection::getBoundingVolumeCenter(
      pRootTile->getBoundingVolume());

  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;

  // Build an ENU frame at the OBB center and offset the camera 10 m east and
  // 0.5 km up, so the look direction is never antiparallel to the surface
  // normal.
  LocalHorizontalCoordinateSystem enu(
      obbCenter,
      LocalDirection::East,
      LocalDirection::North,
      LocalDirection::Up,
      1.0,
      ellipsoid);
  const glm::dvec3 camPos = enu.localPositionToEcef({10.0, 0.0, 500.0});
  const glm::dvec3 direction = glm::normalize(obbCenter - camPos);
  const glm::dvec3 up = ellipsoid.geodeticSurfaceNormal(camPos);

  constexpr double kHFov = CesiumUtility::Math::degreesToRadians(75.0);
  constexpr glm::dvec2 kViewport{1280.0, 720.0};
  const double vFov =
      std::atan(std::tan(kHFov * 0.5) / (kViewport.x / kViewport.y)) * 2.0;

  ViewState viewState(camPos, direction, up, kViewport, kHFov, vFov, ellipsoid);

  TilesetViewGroup viewGroup;
  const ViewUpdateResult& result =
      tileset.updateViewGroupOffline(viewGroup, {viewState});

  bool hasGltfContent = false;
  SPDLOG_INFO(
      "Tiles rendered: {}, worker thread load queue length: {}, main thread "
      "load "
      "queue length: {}",
      result.tilesToRenderThisFrame.size(),
      result.workerThreadTileLoadQueueLength,
      result.mainThreadTileLoadQueueLength);
  for (const Tile::ConstPointer& pTile : result.tilesToRenderThisFrame) {
    if (!pTile) {
      continue;
    }
    const TileRenderContent* rc = pTile->getContent().getRenderContent();
    if (rc && !rc->getModel().meshes.empty()) {
      hasGltfContent = true;
      break;
    }
  }

  if (!hasGltfContent) {
    WARN("No GLTF meshes rendered — the service may be unreachable or the "
         "geometry format is not yet supported.");
  }

  CHECK(hasGltfContent);
}

#endif // CESIUM_DISABLE_CURL
