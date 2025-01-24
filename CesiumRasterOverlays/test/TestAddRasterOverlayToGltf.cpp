#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/TextureInfo.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/ImageManipulation.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfWriter/GltfWriter.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/StringHelpers.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
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
          Cartographic::fromDegrees(-75.14777, 39.95021, 200.0)),
      Ellipsoid::WGS84);
  glm::dmat4 scale =
      glm::scale(glm::dmat4(1.0), glm::dvec3(100000.0, 100000.0, 100000.0));
  glm::dmat4 modelToEcef = enuToFixed * scale;

  // Find the first unused texture coordinate set across all
  // primitives.
  int32_t textureCoordinateIndex = 0;
  for (const Mesh& mesh : gltf.meshes) {
    for (const MeshPrimitive& primitive : mesh.primitives) {
      while (primitive.attributes.find(
                 "TEXCOORD_" + std::to_string(textureCoordinateIndex)) !=
             primitive.attributes.end()) {
        ++textureCoordinateIndex;
      }
    }
  }

  // Set up some mock resources for the raster overlay.
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
    std::string url = "file:///" + StringHelpers::toStringUtf8(
                                       entry.path().generic_u8string());
    auto pRequest = std::make_unique<SimpleAssetRequest>(
        "GET",
        url,
        CesiumAsync::HttpHeaders{},
        std::move(pResponse));
    mapUrlToRequest[url] = std::move(pRequest);
  }

  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  // Create the raster overlay to drape over the glTF.
  std::string tmr =
      "file:///" +
      StringHelpers::toStringUtf8(
          std::filesystem::directory_entry(
              dataDir / "Cesium_Logo_Color" / "tilemapresource.xml")
              .path()
              .generic_u8string());
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
          .thenInMainThread([&gltf, &modelToEcef, textureCoordinateIndex](
                                RasterOverlay::CreateTileProviderResult&&
                                    tileProviderResult) {
            REQUIRE(tileProviderResult);

            IntrusivePointer<RasterOverlayTileProvider> pTileProvider =
                *tileProviderResult;

            std::optional<RasterOverlayDetails> details =
                RasterOverlayUtilities::createRasterOverlayTextureCoordinates(
                    gltf,
                    modelToEcef,
                    std::nullopt,
                    {pTileProvider->getProjection()},
                    true,
                    "TEXCOORD_",
                    textureCoordinateIndex);
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
                    16.0, // the Max SSE used to render the geometry
                    details->rasterOverlayProjections[0],
                    details->rasterOverlayRectangles[0],
                    Ellipsoid::WGS84);

            // Get a raster overlay texture of the proper dimensions.
            IntrusivePointer<RasterOverlayTile> pRasterTile =
                pTileProvider->getTile(
                    details->rasterOverlayRectangles[0],
                    targetScreenPixels);

            glm::dvec4 textureTranslationAndScale =
                RasterOverlayUtilities::computeTranslationAndScale(
                    details->rasterOverlayRectangles[0],
                    pRasterTile->getRectangle());

            // Go load the texture.
            return pTileProvider->loadTile(*pRasterTile)
                .thenPassThrough(std::move(textureTranslationAndScale));
          })
          .thenInMainThread([&gltf, textureCoordinateIndex](
                                std::tuple<glm::dvec4, TileProviderAndTile>&&
                                    tuple) {
            auto& [textureTranslationAndScale, loadResult] = tuple;

            // Create the image, sampler, and texture for the raster overlay
            Image& image = gltf.images.emplace_back();
            image.mimeType = Image::MimeType::image_png;

            Sampler& sampler = gltf.samplers.emplace_back();
            sampler.magFilter = Sampler::MagFilter::LINEAR;
            sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
            sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
            sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

            Texture& texture = gltf.textures.emplace_back();
            texture.sampler = int32_t(gltf.samplers.size() - 1);
            texture.source = int32_t(gltf.images.size() - 1);

            Buffer& buffer = !gltf.buffers.empty()
                                 ? gltf.buffers.front()
                                 : gltf.buffers.emplace_back();
            size_t imageStart = buffer.cesium.data.size();

            // PNG-encode the raster overlay image and store it in the main
            // buffer.
            ImageManipulation::savePng(
                *loadResult.pTile->getImage(),
                buffer.cesium.data);

            BufferView& bufferView = gltf.bufferViews.emplace_back();
            bufferView.buffer = 0;
            bufferView.byteOffset = int64_t(imageStart);
            bufferView.byteLength =
                int64_t(buffer.cesium.data.size() - imageStart);

            buffer.byteLength = int64_t(buffer.cesium.data.size());

            image.bufferView = int32_t(gltf.bufferViews.size() - 1);

            // The below will replace any existing color texture with
            // the raster overlay, because glTF only allows one color
            // texture. However, it doesn't clean up previous textures or
            // texture coordinates, leaving the model bigger than it needs
            // to be. If this were production code (not just a test/demo), we
            // would want to address this.

            int32_t newMaterialIndex = -1;

            for (Mesh& mesh : gltf.meshes) {
              for (MeshPrimitive& primitive : mesh.primitives) {
                if (primitive.material < 0 ||
                    size_t(primitive.material) >= gltf.materials.size()) {
                  // This primitive didn't previous have a material so
                  // assign one (creating it if needed).
                  if (newMaterialIndex < 0) {
                    newMaterialIndex = int32_t(gltf.materials.size());
                    Material& material = gltf.materials.emplace_back();
                    MaterialPBRMetallicRoughness& pbr =
                        material.pbrMetallicRoughness.emplace();
                    pbr.metallicFactor = 0.0;
                    pbr.roughnessFactor = 1.0;
                  }
                  primitive.material = newMaterialIndex;
                }

                Material& material = gltf.materials[size_t(primitive.material)];
                if (!material.pbrMetallicRoughness)
                  material.pbrMetallicRoughness.emplace();
                if (!material.pbrMetallicRoughness->baseColorTexture)
                  material.pbrMetallicRoughness->baseColorTexture.emplace();

                TextureInfo& colorTexture =
                    *material.pbrMetallicRoughness->baseColorTexture;
                colorTexture.index = int32_t(gltf.textures.size() - 1);
                colorTexture.texCoord = textureCoordinateIndex;

                ExtensionKhrTextureTransform& textureTransform =
                    colorTexture.addExtension<ExtensionKhrTextureTransform>();
                textureTransform.offset = {
                    textureTranslationAndScale.x,
                    textureTranslationAndScale.y};
                textureTransform.scale = {
                    textureTranslationAndScale.z,
                    textureTranslationAndScale.w};
              }
            }
          });

  waitForFuture(asyncSystem, std::move(future));

  GltfUtilities::collapseToSingleBuffer(gltf);

  GltfWriterOptions options;
  options.prettyPrint = true;

  GltfWriter writer;
  GltfWriterResult writeResult =
      writer.writeGlb(gltf, gltf.buffers[0].cesium.data, options);

  // Read it back and verify everything still looks good.
  GltfReaderResult resultBack = reader.readGltf(writeResult.gltfBytes);
  REQUIRE(resultBack.model);

  const Model& gltfBack = *resultBack.model;

  REQUIRE(gltfBack.images.size() == 1);
  REQUIRE(gltfBack.images[0].pAsset != nullptr);
  CHECK(!gltfBack.images[0].pAsset->pixelData.empty());

  REQUIRE(!gltfBack.meshes.empty());
  REQUIRE(!gltfBack.meshes[0].primitives.empty());

  const MeshPrimitive& primitive = gltfBack.meshes[0].primitives[0];

  auto texCoordIt = primitive.attributes.find("TEXCOORD_0");
  REQUIRE(texCoordIt != primitive.attributes.end());

  AccessorView<glm::vec2> texCoordView(gltfBack, texCoordIt->second);
  CHECK(texCoordView.size() > 0);

  for (int64_t i = 0; i < texCoordView.size(); ++i) {
    CHECK(texCoordView[i].x >= 0.0);
    CHECK(texCoordView[i].x <= 1.0);
    CHECK(texCoordView[i].y >= 0.0);
    CHECK(texCoordView[i].y <= 1.0);
  }
}
