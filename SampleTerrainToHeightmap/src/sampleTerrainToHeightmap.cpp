#include "ThreadPoolTaskProcessor.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumCurl/UrlAssetAccessor.h>
#include <CesiumGltfContent/ImageManipulation.h>

#include <charconv>
#include <iostream>
#include <string>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumCurl;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfContent;
using namespace CesiumUtility;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "sampleTerrainToHeightmap <Cesium ion Asset ID> <Cesium ion "
                 "Access Token>"
              << std::endl;
    return 1;
  }

  std::string assetIdString = argv[1];
  int64_t assetId;
  std::from_chars(
      assetIdString.data(),
      assetIdString.data() + assetIdString.size(),
      assetId);
  std::string accessToken = argv[2];

  TilesetExternals externals{
      .pAssetAccessor = std::make_shared<UrlAssetAccessor>(),
      .pPrepareRendererResources = nullptr,
      .asyncSystem = AsyncSystem(std::make_shared<ThreadPoolTaskProcessor>()),
      .pCreditSystem = std::make_shared<CreditSystem>(),
      .pLogger = spdlog::default_logger(),
      .pTileOcclusionProxyPool = nullptr,
      .pSharedAssetSystem = TilesetSharedAssetSystem::getDefault()};

  Tileset tileset(externals, assetId, accessToken);

  size_t width = 100;
  size_t height = 100;
  GlobeRectangle rectangle =
      GlobeRectangle::fromDegrees(-78.0, 40.0, -77.0, 41.0);
  //   GlobeRectangle rectangle =
  //       GlobeRectangle::fromDegrees(-78.0, 40.0, -77.9, 40.1);
  double rectangleWidth = rectangle.computeWidth();
  double rectangleHeight = rectangle.computeHeight();

  std::vector<Cartographic> pointsToSample;
  pointsToSample.reserve(width * height);

  for (size_t j = 0; j < height; ++j) {
    double latitude = rectangle.getSouth() + rectangleHeight * j / (height - 1);
    for (size_t i = 0; i < width; ++i) {
      double longitude = rectangle.getWest() + rectangleWidth * i / (width - 1);
      pointsToSample.emplace_back(Cartographic(longitude, latitude, 0.0));
    }
  }

  Future<void> future =
      tileset.sampleHeightMostDetailed(pointsToSample)
          .thenInMainThread([&](SampleHeightResult&& result) {
            std::cout << "Warning Count: " << result.warnings.size()
                      << std::endl;
            std::cout << "Failed samples: "
                      << std::count(
                             result.sampleSuccess.begin(),
                             result.sampleSuccess.end(),
                             false)
                      << std::endl;

            // Write out the heightmap as a PNG image.
            // This will be lossy because of the 8-bit pixels,
            // so use a different format if doing this for
            // real.
            ImageAsset image;
            image.width = width;
            image.height = height;
            image.bytesPerChannel = 1;
            image.channels = 1;
            image.pixelData.resize(width * height);

            double maxHeight = -10000.0;
            double minHeight = 10000.0;
            for (size_t i = 0; i < result.positions.size(); ++i) {
              maxHeight = std::max(maxHeight, result.positions[i].height);
              minHeight = std::min(minHeight, result.positions[i].height);
            }

            for (size_t i = 0; i < result.positions.size(); ++i) {
              double height = result.positions[i].height;
              image.pixelData[i] = std::byte(
                  std::round(
                      255.0 * (height - minHeight) / (maxHeight - minHeight)));
            }

            std::vector<std::byte> buffer;
            ImageManipulation::savePng(image, buffer);

            FILE* fp = std::fopen("out.png", "wb");
            std::fwrite(buffer.data(), 1, buffer.size(), fp);
            std::fclose(fp);
          });

  while (!future.isReady()) {
    tileset.loadTiles();
    std::this_thread::yield();
  }

  return 0;
}
