#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/UrlTemplateRasterOverlay.h>

#include <doctest/doctest.h>

#include <filesystem>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace CesiumNativeTests;
using namespace CesiumRasterOverlays;

TEST_CASE("UrlTemplateRasterOverlay getTile") {
  std::filesystem::path dataDir(CesiumRasterOverlays_TEST_DATA_DIR);
  std::vector<std::byte> blackPng = readFile(dataDir / "black.png");

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};
  auto pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());
  pAssetAccessor->mockCompletedRequests.insert(
      {"http://example.com/0/0/0.png",
       std::make_shared<SimpleAssetRequest>(
           "GET",
           "http://example.com/0/0/0.png",
           HttpHeaders{},
           std::make_unique<SimpleAssetResponse>(
               uint16_t(200),
               "doesn't matter",
               HttpHeaders{},
               blackPng))});

  IntrusivePointer<UrlTemplateRasterOverlay> pOverlay =
      new UrlTemplateRasterOverlay(
          "Test",
          "http://example.com/{x}/{y}/{z}.png");

  IntrusivePointer<RasterOverlayTileProvider> pProvider = nullptr;

  pOverlay
      ->createTileProvider(
          asyncSystem,
          pAssetAccessor,
          nullptr,
          nullptr,
          spdlog::default_logger(),
          nullptr)
      .thenInMainThread(
          [&pProvider](RasterOverlay::CreateTileProviderResult&& created) {
            CHECK(created);
            pProvider = *created;
          });

  asyncSystem.dispatchMainThreadTasks();

  REQUIRE(pProvider);
  REQUIRE(!pProvider->isPlaceholder());

  Rectangle rectangle =
      GeographicProjection::computeMaximumProjectedRectangle(Ellipsoid::WGS84);
  IntrusivePointer<RasterOverlayTile> pTile =
      pProvider->getTile(rectangle, glm::dvec2(256));
  pProvider->loadTile(*pTile);

  while (pTile->getState() != RasterOverlayTile::LoadState::Loaded) {
    asyncSystem.dispatchMainThreadTasks();
  }

  CHECK(pTile->getState() == RasterOverlayTile::LoadState::Loaded);

  REQUIRE(pTile->getImage());

  const ImageAsset& image = *pTile->getImage();
  CHECK(image.width > 0);
  CHECK(image.height > 0);
  CHECK(image.pixelData.size() > 0);
  CHECK(std::all_of(
      image.pixelData.begin(),
      image.pixelData.end(),
      [](std::byte b) { return b == std::byte(0); }));
}
