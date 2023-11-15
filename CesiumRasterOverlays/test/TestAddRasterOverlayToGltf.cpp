#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfWriter/GltfWriter.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <catch2/catch.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfContent;
using namespace CesiumGltfReader;
using namespace CesiumGltfWriter;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

TEST_CASE("Add raster overlay to glTF") {
  std::filesystem::path dataDir(CesiumRasterOverlays_TEST_DATA_DIR);
  std::vector<std::byte> bytes = readFile(dataDir / "Shadow_Tester.glb");
  GltfReader reader;
  GltfReaderResult result = reader.readGltf(bytes);
  REQUIRE(result.model);

  Model& gltf = *result.model;

  // Place the glTF in Philly and make it huge.
  glm::dmat4 enuToFixed = GlobeTransforms::eastNorthUpToFixedFrame(
      Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic::fromDegrees(-75.14777, 39.95021, 200.0)));
  glm::dmat4 scale =
      glm::scale(glm::dmat4(1.0), glm::dvec3(100000.0, 100000.0, 100000.0));
  glm::dmat4 modelToEcef = enuToFixed * scale;

  // Set up some mock resources.
  auto pMockTaskProcessor = std::make_shared<SimpleTaskProcessor>();
  CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(
           dataDir / "Cesium_Logo_Color")) {
    if (!entry.is_regular_file())
      continue;
    auto pResponse = std::make_unique<SimpleAssetResponse>(
        uint16_t(200),
        "application/binary",
        CesiumAsync::HttpHeaders{},
        readFile(entry.path()));
    std::string url = "file:///" + entry.path().generic_u8string();
    auto pRequest = std::make_unique<SimpleAssetRequest>(
        "GET",
        url,
        CesiumAsync::HttpHeaders{},
        std::move(pResponse));
    mapUrlToRequest[url] = std::move(pRequest);
  }

  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  // Create a raster overlay to drape over the glTF.
  std::string tmr =
      "file:///" + std::filesystem::directory_entry(
                       dataDir / "Cesium_Logo_Color" / "tilemapresource.xml")
                       .path()
                       .generic_u8string();
  IntrusivePointer<TileMapServiceRasterOverlay> pRasterOverlay =
      new TileMapServiceRasterOverlay("test", tmr);

  auto future =
      pRasterOverlay
          ->createTileProvider(
              asyncSystem,
              pMockAssetAccessor,
              nullptr,
              nullptr,
              spdlog::default_logger(),
              nullptr)
          .thenInMainThread([&gltf, &modelToEcef](
                                RasterOverlay::CreateTileProviderResult&&
                                    tileProviderResult) {
            REQUIRE(tileProviderResult);

            IntrusivePointer<RasterOverlayTileProvider> pTileProvider =
                tileProviderResult.value();

            std::optional<RasterOverlayDetails> details =
                RasterOverlayUtilities::createRasterOverlayTextureCoordinates(
                    gltf,
                    modelToEcef,
                    0,
                    std::nullopt,
                    {pTileProvider->getProjection()});
            REQUIRE(details);
            REQUIRE(details->rasterOverlayProjections.size() == 1);
            REQUIRE(details->rasterOverlayRectangles.size() == 1);

            // The geometric error will usually come from the tile, but here
            // we're hard-coding it.
            double geometricError = 100000.0;

            // Determine the maximum number of screen pixels we expect to be
            // covered by this raster overlay.
            glm::dvec2 targetScreenPixels =
                RasterOverlayUtilities::computeDesiredScreenPixels(
                    geometricError,
                    16.0,
                    details->rasterOverlayProjections[0],
                    details->rasterOverlayRectangles[0]);

            // Get a raster overlay texture of the proper dimensions.
            IntrusivePointer<RasterOverlayTile> pRasterTile =
                pTileProvider->getTile(
                    details->rasterOverlayRectangles[0],
                    targetScreenPixels);

            // And go load the texture.
            return pTileProvider->loadTile(*pRasterTile);
          })
          .thenInMainThread([&gltf](TileProviderAndTile&& loadResult) {
            Image& image = gltf.images.emplace_back();
            image.cesium = loadResult.pTile->getImage();

            Sampler& sampler = gltf.samplers.emplace_back();
            sampler.magFilter = Sampler::MagFilter::LINEAR;
            sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
            sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
            sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

            Texture& texture = gltf.textures.emplace_back();
            texture.sampler = int32_t(gltf.samplers.size() - 1);
            texture.source = int32_t(gltf.images.size() - 1);
          });

  waitForFuture(asyncSystem, std::move(future));

  GltfWriter writer;
  GltfWriterResult writeResult =
      writer.writeGlb(gltf, gltf.buffers[0].cesium.data);
}
