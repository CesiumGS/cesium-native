#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <doctest/doctest.h>
#include <glm/fwd.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>

using namespace CesiumGeospatial;
using namespace CesiumVectorData;

TEST_CASE("VectorRasterizer::rasterize") {
  GlobeRectangle rect{0.0, 0.0, 1.0, 1.0};

  SUBCASE("Renders a single triangle") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels *
                 asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect, asset);

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(0.25, 0.25),
        glm::dvec2(0.5, 0.75),
        glm::dvec2(0.75, 0.25)});

    rasterizer.drawPolygon(
        triangle,
        Color{std::byte{0}, std::byte{255}, std::byte{255}, std::byte{255}});
    rasterizer.finalize();
    asset->writeTga("triangle.tga");
  }

  SUBCASE("Renders a polyline") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels *
                 asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect, asset);

    std::vector<Cartographic> polyline {
        Cartographic(0.25, 0.25),
        Cartographic(0.25, 0.5),
        Cartographic(0.3, 0.7),
        Cartographic(0.25, 0.8),
        Cartographic(0.8, 1.0),
        Cartographic(0.8, 0.9),
        Cartographic(0.9, 0.9)
    };

    rasterizer.drawPolyline(
        polyline,
        Color{std::byte{0}, std::byte{255}, std::byte{255}, std::byte{255}});
    rasterizer.finalize();
    asset->writeTga("polyline.tga");
  }
}

TEST_CASE("VectorRasterizer::rasterize benchmark") {
  GlobeRectangle rect{0.0, 0.0, 1.0, 1.0};
  std::chrono::steady_clock clock;
  std::random_device r;
  std::default_random_engine rand(r());

  std::chrono::microseconds total(0);

  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
  asset.emplace();
  asset->width = 256;
  asset->height = 256;
  asset->channels = 4;
  asset->bytesPerChannel = 1;
  asset->pixelData.resize(
      (size_t)(asset->width * asset->height * asset->channels *
               asset->bytesPerChannel),
      std::byte{255});

  for (int i = 0; i < 100; i++) {
    std::vector<CartographicPolygon> polygons;
    std::vector<Color> colors;
    std::uniform_real_distribution<double> uniformDist;
    for (int j = 0; j < 1000; j++) {
      polygons.emplace_back(std::vector<glm::dvec2>{
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
      });
      colors.emplace_back(Color{
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0)});
    }

    std::chrono::steady_clock::time_point start = clock.now();

    VectorRasterizer rasterizer(rect, asset);
    for (size_t j = 0; j < polygons.size(); j++) {
      rasterizer.drawPolygon(polygons[j], colors[j]);
    }
    rasterizer.finalize();

    std::chrono::microseconds time =
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock.now() - start);
    total += time;
    std::cout << "rasterized 1000 triangles in " << time.count()
              << " microseconds\n";
    asset->writeTga("rand.tga");
  }

  double seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(total).count();
  std::cout << "100 runs in " << seconds << " seconds, avg per run "
            << (seconds / 100.0) << " seconds\n";
}