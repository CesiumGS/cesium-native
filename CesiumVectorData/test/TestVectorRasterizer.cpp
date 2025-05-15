#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <doctest/doctest.h>
#include <fmt/format.h>
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
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect, asset);

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(0.25, 0.25),
        glm::dvec2(0.5, 0.75),
        glm::dvec2(0.75, 0.25)});

    rasterizer.drawPolygon(
        triangle,
        VectorStyle{Color{
            std::byte{0},
            std::byte{255},
            std::byte{255},
            std::byte{255}}});
    rasterizer.finalize();
    asset->writeTga("triangle.tga");
  }

  SUBCASE("Triangle as tiles lines up") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(0.25, 0.25),
        glm::dvec2(0.5, 0.75),
        glm::dvec2(0.75, 0.25)});
    VectorStyle style{
        Color{std::byte{0}, std::byte{255}, std::byte{255}, std::byte{255}}};
    {
      VectorRasterizer rasterizer(rect, asset);

      rasterizer.drawPolygon(triangle, style);
      rasterizer.finalize();
    }

    for (size_t i = 0; i < 2; i++) {
      for (size_t j = 0; j < 2; j++) {
        CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> tile;
        tile.emplace();
        tile->width = 128;
        tile->height = 128;
        tile->channels = 4;
        tile->bytesPerChannel = 1;
        tile->pixelData.resize(
            (size_t)(tile->width * tile->height * tile->channels * tile->bytesPerChannel),
            std::byte{255});
        VectorRasterizer rasterizer(
            GlobeRectangle(
                (double)i * 0.5,
                (double)j * 0.5,
                0.5 + (double)i * 0.5,
                0.5 + (double)j * 0.5),
            tile);
        rasterizer.drawPolygon(triangle, style);
        rasterizer.finalize();

        tile->writeTga(fmt::format("tile-{}-{}.tga", i, j));

        for (size_t x = 0; x < (size_t)128; x++) {
          for (size_t y = 0; y < (size_t)128; y++) {
            CHECK(
                tile->pixelData[(x * 128 + y) * 4] ==
                asset->pixelData[((x + i * 128) * 256 + (y + j * 128)) * 4]);
          }
        }
      }
    }
  }

  SUBCASE("Renders a polyline") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect, asset);

    std::vector<Cartographic> polyline{
        Cartographic(0.25, 0.25),
        Cartographic(0.25, 0.5),
        Cartographic(0.3, 0.7),
        Cartographic(0.25, 0.8),
        Cartographic(0.8, 1.0),
        Cartographic(0.8, 0.9),
        Cartographic(0.9, 0.9)};

    rasterizer.drawPolyline(
        polyline,
        VectorStyle{Color{
            std::byte{81},
            std::byte{33},
            std::byte{255},
            std::byte{255}}});
    rasterizer.finalize();
    asset->writeTga("polyline.tga");
  }

  SUBCASE("Transforms bounds properly") {
    GlobeRectangle rect2(0.25, 0.25, 0.75, 0.5);
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect2, asset);

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(0.375, 0.3125),
        glm::dvec2(0.5, 0.4375),
        glm::dvec2(0.625, 0.3125)});

    rasterizer.drawPolygon(
        triangle,
        VectorStyle{Color{
            std::byte{255},
            std::byte{127},
            std::byte{100},
            std::byte{255}}});
    rasterizer.finalize();
    asset->writeTga("triangle-scaled.tga");
  }

  SUBCASE("Polygon with holes") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    VectorRasterizer rasterizer(rect, asset);

    std::vector<std::vector<Cartographic>> composite{
        std::vector<Cartographic>{
            Cartographic(0.25, 0.25),
            Cartographic(0.75, 0.25),
            Cartographic(0.75, 0.75),
            Cartographic(0.25, 0.75),
            Cartographic(0.25, 0.25)},
        std::vector<Cartographic>{
            Cartographic(0.4, 0.4),
            Cartographic(0.4, 0.6),
            Cartographic(0.6, 0.6),
            Cartographic(0.6, 0.4),
            Cartographic(0.4, 0.4)}};

    rasterizer.drawPolygon(
        composite,
        VectorStyle{Color{
            std::byte{255},
            std::byte{50},
            std::byte{12},
            std::byte{255}}});
    rasterizer.finalize();
    asset->writeTga("polygon-holes.tga");
  }

  SUBCASE("Mip levels") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->mipPositions.resize(4);
    size_t totalSize = 0;
    for (size_t i = 0; i < 4; i++) {
      int32_t width = 256 >> i;
      int32_t height = 256 >> i;
      asset->mipPositions[i] = {totalSize, (size_t)(width * height * 4)};
      totalSize += asset->mipPositions[i].byteSize;
    }

    asset->pixelData.resize(totalSize, std::byte{255});

    std::vector<Cartographic> polyline{
        Cartographic(0.25, 0.25),
        Cartographic(0.25, 0.5),
        Cartographic(0.3, 0.7),
        Cartographic(0.25, 0.8),
        Cartographic(0.8, 1.0),
        Cartographic(0.8, 0.9),
        Cartographic(0.9, 0.9)};
    VectorStyle style{
        Color{std::byte{255}, std::byte{50}, std::byte{12}, std::byte{255}}};

    for (uint32_t i = 0; i < 4; i++) {
      VectorRasterizer rasterizer(rect, asset, i);
      rasterizer.drawPolyline(polyline, style);
      rasterizer.finalize();
    }

    asset->writeTga("mipmap.tga");
  }

  SUBCASE("Styling") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 256;
    asset->height = 256;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(
        (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
        std::byte{255});

    CartographicPolygon square(std::vector<glm::dvec2>{
        glm::dvec2(0.25, 0.25),
        glm::dvec2(0.25, 0.75),
        glm::dvec2(0.75, 0.75),
        glm::dvec2(0.75, 0.25)});

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(0.25, 0.25),
        glm::dvec2(0.5, 0.75),
        glm::dvec2(0.75, 0.25)});

    std::vector<Cartographic> polyline{
        Cartographic(0.1, 0.1),
        Cartographic(0.9, 0.1),
        Cartographic(0.9, 0.9)};

    VectorStyle style{
        LineStyle{ColorStyle{Color{0xff, 0xaa, 0x33}, ColorMode::Normal}, 2.0},
        PolygonStyle{
            ColorStyle{Color{0x33, 0xaa, 0xff}, ColorMode::Random},
            true,
            true}};

    VectorRasterizer rasterizer(rect, asset);
    rasterizer.drawPolygon(square, style);
    rasterizer.drawPolygon(triangle, style);
    rasterizer.drawPolyline(polyline, style);
    rasterizer.finalize();

    asset->writeTga("styling.tga");
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
      (size_t)(asset->width * asset->height * asset->channels * asset->bytesPerChannel),
      std::byte{255});

  for (int i = 0; i < 100; i++) {
    std::vector<CartographicPolygon> polygons;
    std::vector<VectorStyle> styles;
    std::uniform_real_distribution<double> uniformDist;
    for (int j = 0; j < 1000; j++) {
      polygons.emplace_back(std::vector<glm::dvec2>{
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
          glm::dvec2(uniformDist(rand), uniformDist(rand)),
      });
      styles.emplace_back(Color{
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0),
          (std::byte)(uniformDist(rand) * 255.0)});
    }

    std::chrono::steady_clock::time_point start = clock.now();

    VectorRasterizer rasterizer(rect, asset);
    for (size_t j = 0; j < polygons.size(); j++) {
      rasterizer.drawPolygon(polygons[j], styles[j]);
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