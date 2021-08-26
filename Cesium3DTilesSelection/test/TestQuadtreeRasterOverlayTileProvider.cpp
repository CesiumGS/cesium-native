#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "SimpleAssetAccessor.h"
#include <catch2/catch.hpp>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

class TestTileProvider : public QuadtreeRasterOverlayTileProvider {
public:
  TestTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t imageWidth,
      uint32_t imageHeight) noexcept
      : QuadtreeRasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            imageWidth,
            imageHeight) {}

  // The tiles that will return an error from loadQuadtreeTileImage.
  std::vector<QuadtreeTileID> errorTiles;

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadQuadtreeTileImage(const QuadtreeTileID& tileID) const {
    if (std::find(errorTiles.begin(), errorTiles.end(), tileID) != errorTiles.end()) {
      LoadedRasterOverlayImage result;
      result.errors.emplace_back("Tile errored.");
      return this->getAsyncSystem().createResolvedFuture(std::move(result));
    }

    // Return an image where every component of every pixel is equal to the tile level.
    LoadedRasterOverlayImage result;
    result.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    result.image = ImageCesium();
    result.image->width = int32_t(this->getWidth());
    result.image->height = int32_t(this->getHeight());
    result.image->bytesPerChannel = 1;
    result.image->channels = 4;
    result.image->pixelData.resize(
        this->getWidth() * this->getHeight() * 4,
        std::byte(tileID.level));
    return this->getAsyncSystem().createResolvedFuture(std::move(result));
  }
};

class TestRasterOverlay : public RasterOverlay {
public:
  TestRasterOverlay(
      const std::string& name,
      const RasterOverlayOptions& options = RasterOverlayOptions())
      : RasterOverlay(name, options) {}

  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& /* pCreditSystem */,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) override {
    if (!pOwner) {
      pOwner = this;
    }

    return asyncSystem
        .createResolvedFuture<std::unique_ptr<RasterOverlayTileProvider>>(
            std::make_unique<TestTileProvider>(
                *pOwner,
                asyncSystem,
                pAssetAccessor,
                std::nullopt,
                pPrepareRendererResources,
                pLogger,
                WebMercatorProjection(),
                QuadtreeTilingScheme(
                    WebMercatorProjection::computeMaximumProjectedRectangle(),
                    1,
                    1),
                WebMercatorProjection::computeMaximumProjectedRectangle(),
                0,
                10,
                256,
                256));
  }
};

class MockTaskProcessor : public ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) { std::thread(f).detach(); }
};

} // namespace

TEST_CASE("QuadtreeRasterOverlayTileProvider getTile") {
  auto pTaskProcessor = std::make_shared<MockTaskProcessor>();
  auto pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());

  AsyncSystem asyncSystem(pTaskProcessor);
  TestRasterOverlay overlay("Test");

  std::unique_ptr<RasterOverlayTileProvider> pProvider =
      overlay
          .createTileProvider(
              asyncSystem,
              pAssetAccessor,
              nullptr,
              nullptr,
              spdlog::default_logger(),
              nullptr)
          .wait();

  // auto pQuadtreeProvider = static_cast<TestTileProvider*>(pProvider.get());

  SECTION("returns deepest tiles for geometric error of 0.0") {
    Rectangle rectangle(0.000001, 0.000001, 0.000002, 0.000002);
    double geometricError = 0.0;
    IntrusivePointer<RasterOverlayTile> pTile = pProvider->getTile(rectangle, geometricError);
    pProvider->loadTile(*pTile);

    for (int i = 0; i < 10 && pTile->getState() != RasterOverlayTile::LoadState::Done; ++i) {
      asyncSystem.dispatchMainThreadTasks();
    }

    CHECK(pTile->getState() == RasterOverlayTile::LoadState::Done);
  }
}
