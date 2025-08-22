#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/writeTga.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <doctest/doctest.h>
#include <fmt/format.h>
#include <glm/fwd.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <random>

using namespace CesiumGeospatial;
using namespace CesiumVectorData;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

namespace {
void checkFilesEqual(
    const std::filesystem::path& fileA,
    const std::filesystem::path& fileB) {
  const std::vector<std::byte>& bytes = readFile(fileA);
  const std::vector<std::byte>& bytes2 = readFile(fileB);

  REQUIRE(bytes.size() == bytes2.size());
  for (size_t i = 0; i < bytes.size(); i++) {
    CHECK(bytes[i] == bytes2[i]);
  }
}
} // namespace

TEST_CASE("VectorRasterizer::rasterize") {
  const std::filesystem::path dir(
      std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "rasterized");
  const std::filesystem::path thisDir = std::filesystem::current_path();

  GlobeRectangle rect{
      0.0,
      0.0,
      Math::degreesToRadians(1.0),
      Math::degreesToRadians(1.0)};

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
        glm::dvec2(Math::degreesToRadians(0.25), Math::degreesToRadians(0.25)),
        glm::dvec2(Math::degreesToRadians(0.5), Math::degreesToRadians(0.75)),
        glm::dvec2(
            Math::degreesToRadians(0.75),
            Math::degreesToRadians(0.25))});

    rasterizer.drawPolygon(
        triangle,
        VectorStyle{Color{0, 255, 255, 255}}.polygon);
    rasterizer.finalize();
    writeImageToTgaFile(*asset, "triangle.tga");
    checkFilesEqual(dir / "triangle.tga", thisDir / "triangle.tga");
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
        glm::dvec2(Math::degreesToRadians(0.25), Math::degreesToRadians(0.25)),
        glm::dvec2(Math::degreesToRadians(0.5), Math::degreesToRadians(0.75)),
        glm::dvec2(
            Math::degreesToRadians(0.75),
            Math::degreesToRadians(0.25))});
    VectorStyle style{Color{0, 255, 255, 255}};
    {
      VectorRasterizer rasterizer(rect, asset);

      rasterizer.drawPolygon(triangle, style.polygon);
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
                Math::degreesToRadians((double)i * 0.5),
                Math::degreesToRadians((double)j * 0.5),
                Math::degreesToRadians(0.5 + (double)i * 0.5),
                Math::degreesToRadians(0.5 + (double)j * 0.5)),
            tile);
        rasterizer.drawPolygon(triangle, style.polygon);
        rasterizer.finalize();

        const std::string fileName = fmt::format("tile-{}-{}.tga", i, j);
        writeImageToTgaFile(*tile, fileName);
        checkFilesEqual(dir / fileName, thisDir / fileName);

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

    std::vector<glm::dvec3> polyline{
        glm::dvec3(0.25, 0.25, 0.0),
        glm::dvec3(0.25, 0.5, 0.0),
        glm::dvec3(0.3, 0.7, 0.0),
        glm::dvec3(0.25, 0.8, 0.0),
        glm::dvec3(0.8, 1.0, 0.0),
        glm::dvec3(0.8, 0.9, 0.0),
        glm::dvec3(0.9, 0.9, 0.0)};

    rasterizer.drawPolyline(
        polyline,
        VectorStyle{Color{81, 33, 255, 255}}.line);
    rasterizer.finalize();
    writeImageToTgaFile(*asset, "polyline.tga");
    checkFilesEqual(dir / "polyline.tga", thisDir / "polyline.tga");
  }

  SUBCASE("Transforms bounds properly") {
    GlobeRectangle rect2(
        Math::degreesToRadians(0.25),
        Math::degreesToRadians(0.25),
        Math::degreesToRadians(0.75),
        Math::degreesToRadians(0.5));
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
        glm::dvec2(
            Math::degreesToRadians(0.375),
            Math::degreesToRadians(0.3125)),
        glm::dvec2(Math::degreesToRadians(0.5), Math::degreesToRadians(0.4375)),
        glm::dvec2(
            Math::degreesToRadians(0.625),
            Math::degreesToRadians(0.3125))});

    rasterizer.drawPolygon(
        triangle,
        VectorStyle{Color{255, 127, 100, 255}}.polygon);
    rasterizer.finalize();
    writeImageToTgaFile(*asset, "triangle-scaled.tga");
    checkFilesEqual(
        dir / "triangle-scaled.tga",
        thisDir / "triangle-scaled.tga");
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

    std::vector<std::vector<glm::dvec3>> composite{
        std::vector<glm::dvec3>{
            glm::dvec3(0.25, 0.25, 0.0),
            glm::dvec3(0.75, 0.25, 0.0),
            glm::dvec3(0.75, 0.75, 0.0),
            glm::dvec3(0.25, 0.75, 0.0),
            glm::dvec3(0.25, 0.25, 0.0)},
        std::vector<glm::dvec3>{
            glm::dvec3(0.4, 0.4, 0.0),
            glm::dvec3(0.4, 0.6, 0.0),
            glm::dvec3(0.6, 0.6, 0.0),
            glm::dvec3(0.6, 0.4, 0.0),
            glm::dvec3(0.4, 0.4, 0.0)}};

    rasterizer.drawPolygon(
        composite,
        VectorStyle{Color{255, 50, 12, 255}}.polygon);
    rasterizer.finalize();
    writeImageToTgaFile(*asset, "polygon-holes.tga");
    checkFilesEqual(dir / "polygon-holes.tga", thisDir / "polygon-holes.tga");
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

    std::vector<glm::dvec3> polyline{
        glm::dvec3(0.25, 0.25, 0.0),
        glm::dvec3(0.25, 0.5, 0.0),
        glm::dvec3(0.3, 0.7, 0.0),
        glm::dvec3(0.25, 0.8, 0.0),
        glm::dvec3(0.8, 1.0, 0.0),
        glm::dvec3(0.8, 0.9, 0.0),
        glm::dvec3(0.9, 0.9, 0.0)};
    VectorStyle style{Color{255, 50, 12, 255}};

    for (uint32_t i = 0; i < 4; i++) {
      VectorRasterizer rasterizer(rect, asset, i);
      rasterizer.drawPolyline(polyline, style.line);
      rasterizer.finalize();
    }

    writeImageToTgaFile(*asset, "mipmap.tga");
    checkFilesEqual(dir / "mipmap-mip0.tga", thisDir / "mipmap-mip0.tga");
    checkFilesEqual(dir / "mipmap-mip1.tga", thisDir / "mipmap-mip1.tga");
    checkFilesEqual(dir / "mipmap-mip2.tga", thisDir / "mipmap-mip2.tga");
    checkFilesEqual(dir / "mipmap-mip3.tga", thisDir / "mipmap-mip3.tga");
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
        glm::dvec2(Math::degreesToRadians(0.25), Math::degreesToRadians(0.25)),
        glm::dvec2(Math::degreesToRadians(0.25), Math::degreesToRadians(0.75)),
        glm::dvec2(Math::degreesToRadians(0.75), Math::degreesToRadians(0.75)),
        glm::dvec2(
            Math::degreesToRadians(0.75),
            Math::degreesToRadians(0.25))});

    CartographicPolygon triangle(std::vector<glm::dvec2>{
        glm::dvec2(Math::degreesToRadians(0.25), Math::degreesToRadians(0.25)),
        glm::dvec2(Math::degreesToRadians(0.5), Math::degreesToRadians(0.75)),
        glm::dvec2(
            Math::degreesToRadians(0.75),
            Math::degreesToRadians(0.25))});

    std::vector<glm::dvec3> polyline{
        glm::dvec3(0.1, 0.1, 0.0),
        glm::dvec3(0.9, 0.1, 0.0),
        glm::dvec3(0.9, 0.9, 0.0)};

    VectorStyle style{
        LineStyle{ColorStyle{Color{0xff, 0xaa, 0x33}, ColorMode::Normal}, 2.0},
        PolygonStyle{
            ColorStyle{Color{0x33, 0xaa, 0xff}, ColorMode::Normal},
            LineStyle{
                ColorStyle{Color{0xff, 0xaa, 0x33}, ColorMode::Normal},
                2.0}}};

    VectorRasterizer rasterizer(rect, asset);
    rasterizer.drawPolygon(square, style.polygon);
    rasterizer.drawPolygon(triangle, style.polygon);
    rasterizer.drawPolyline(polyline, style.line);
    rasterizer.finalize();

    writeImageToTgaFile(*asset, "styling.tga");
    checkFilesEqual(dir / "styling.tga", thisDir / "styling.tga");
  }

  SUBCASE("Random color uses the same color across calls") {
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> asset;
    asset.emplace();
    asset->width = 1;
    asset->height = 1;
    asset->channels = 4;
    asset->bytesPerChannel = 1;
    asset->pixelData.resize(4, std::byte{255});

    CartographicPolygon square(std::vector<glm::dvec2>{
        glm::dvec2{Math::degreesToRadians(0.0), Math::degreesToRadians(0.0)},
        glm::dvec2{Math::degreesToRadians(0.0), Math::degreesToRadians(1.0)},
        glm::dvec2{Math::degreesToRadians(1.0), Math::degreesToRadians(1.0)},
        glm::dvec2{Math::degreesToRadians(1.0), Math::degreesToRadians(0.0)},
        glm::dvec2{Math::degreesToRadians(0.0), Math::degreesToRadians(0.0)}});

    VectorStyle style;
    style.polygon.fill =
        ColorStyle{Color{0xff, 0x00, 0xaa, 0xff}, ColorMode::Random};

    VectorRasterizer rasterizer(rect, asset);
    rasterizer.drawPolygon(square, style.polygon);
    rasterizer.finalize();

    const uint32_t writtenColor =
        *reinterpret_cast<uint32_t*>(asset->pixelData.data());
    VectorRasterizer rasterizer2(rect, asset);
    rasterizer2.drawPolygon(square, style.polygon);
    rasterizer2.finalize();

    CHECK(
        *reinterpret_cast<uint32_t*>(asset->pixelData.data()) == writtenColor);
  }
}

TEST_CASE("VectorRasterizer::rasterize benchmark" * doctest::skip(true)) {
  GlobeRectangle rect{
      0.0,
      0.0,
      Math::degreesToRadians(1.0),
      Math::degreesToRadians(1.0)};
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
          glm::dvec2(
              Math::degreesToRadians(uniformDist(rand)),
              Math::degreesToRadians(uniformDist(rand))),
          glm::dvec2(
              Math::degreesToRadians(uniformDist(rand)),
              Math::degreesToRadians(uniformDist(rand))),
          glm::dvec2(
              Math::degreesToRadians(uniformDist(rand)),
              Math::degreesToRadians(uniformDist(rand))),
      });
      styles.emplace_back(Color{
          (uint8_t)(uniformDist(rand) * 255.0),
          (uint8_t)(uniformDist(rand) * 255.0),
          (uint8_t)(uniformDist(rand) * 255.0),
          (uint8_t)(uniformDist(rand) * 255.0)});
    }

    std::chrono::steady_clock::time_point start = clock.now();

    VectorRasterizer rasterizer(rect, asset);
    for (size_t j = 0; j < polygons.size(); j++) {
      rasterizer.drawPolygon(polygons[j], styles[j].polygon);
    }
    rasterizer.finalize();

    std::chrono::microseconds time =
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock.now() - start);
    total += time;
    std::cout << "rasterized 1000 triangles in " << time.count()
              << " microseconds\n";
    writeImageToTgaFile(*asset, "rand.tga");
  }

  double seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(total).count();
  std::cout << "100 runs in " << seconds << " seconds, avg per run "
            << (seconds / 100.0) << " seconds\n";
}