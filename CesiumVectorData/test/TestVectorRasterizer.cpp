#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <doctest/doctest.h>
#include <glm/fwd.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>

using namespace CesiumGeospatial;
using namespace CesiumVectorData;

TEST_CASE("VectorRasterizer::rasterize") {
  GlobeRectangle rect{0.0, 0.0, 1.0, 1.0};

  SUBCASE("Generates a single triangle") {
    VectorRasterizer rasterizer(
        std::vector<CartographicPolygon>{
            CartographicPolygon(std::vector<glm::dvec2>{
                glm::dvec2(0.25, 0.25),
                glm::dvec2(0.5, 0.75),
                glm::dvec2(0.75, 0.25)})},
        std::vector<std::array<std::byte, 4>>{std::array<std::byte, 4>{
            std::byte{0},
            std::byte{255},
            std::byte{255},
            std::byte{255}}});

    CesiumGltf::ImageAsset asset;
    asset.width = 256;
    asset.height = 256;
    asset.channels = 4;
    asset.bytesPerChannel = 1;
    asset.pixelData.resize(
        (size_t)(asset.width * asset.height * asset.channels *
                 asset.bytesPerChannel),
        std::byte{255});

    rasterizer.rasterize(rect, asset);
    asset.writeTga("triangle.tga");
  }

  SUBCASE("Alpha blends properly") {
    VectorRasterizer rasterizer(
        std::vector<CartographicPolygon>{
            CartographicPolygon(std::vector<glm::dvec2>{
                glm::dvec2(0.25, 0.25),
                glm::dvec2(0.5, 0.75),
                glm::dvec2(0.75, 0.25)}),
                CartographicPolygon(std::vector<glm::dvec2>{
                    glm::dvec2(0.25, 0.25),
                    glm::dvec2(0.25, 0.75),
                    glm::dvec2(0.9, 0.1)})},
        std::vector<std::array<std::byte, 4>>{
            std::array<std::byte, 4>{
                std::byte{0},
                std::byte{255},
                std::byte{255},
                std::byte{127}},
            std::array<std::byte, 4>{
                std::byte{0},
                std::byte{255},
                std::byte{0},
                std::byte{60}}});

    CesiumGltf::ImageAsset asset;
    asset.width = 256;
    asset.height = 256;
    asset.channels = 4;
    asset.bytesPerChannel = 1;
    asset.pixelData.resize(
        (size_t)(asset.width * asset.height * asset.channels *
                 asset.bytesPerChannel),
        std::byte{255});

    rasterizer.rasterize(rect, asset);
    asset.writeTga("blending.tga");
  }
}

TEST_CASE("VectorRasterizer::rasterize benchmark") {
  GlobeRectangle rect{0.0, 0.0, 1.0, 1.0};
  std::chrono::steady_clock clock;
  std::random_device r;
  std::default_random_engine rand(r());

  std::chrono::microseconds total(0);

  for (int i = 0; i < 100; i++) {
    std::vector<CartographicPolygon> polygons;
    std::vector<std::array<std::byte, 4>> colors;
    std::uniform_real_distribution<double> uniformDist;
    for (int j = 0; j < 1000; j++) {
      polygons.emplace_back(std::vector<glm::dvec2>{
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
      });
      colors.emplace_back(std::array<std::byte, 4>{
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0)});
    }
    VectorRasterizer rasterizer(polygons, colors);

    CesiumGltf::ImageAsset asset;
    asset.width = 256;
    asset.height = 256;
    asset.channels = 4;
    asset.bytesPerChannel = 1;
    asset.pixelData.resize(
        (size_t)(asset.width * asset.height * asset.channels *
                 asset.bytesPerChannel),
        std::byte{255});

    std::chrono::steady_clock::time_point start = clock.now();
    rasterizer.rasterize(rect, asset);
    std::chrono::microseconds time =
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock.now() - start);
    total += time;
    std::cout << "rasterized 1000 triangles in " << time.count()
              << " microseconds\n";
    asset.writeTga("rand.tga");
  }

  double seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(total).count();
  std::cout << "100 runs in " << seconds << " seconds, avg per run "
            << (seconds / 100.0) << " seconds\n";
}