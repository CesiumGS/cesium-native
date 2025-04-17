#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumRasterOverlays/VectorRasterizer.h>

#include <doctest/doctest.h>
#include <glm/fwd.hpp>

#include <array>
#include <cstddef>

using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;

TEST_CASE("VectorRasterizer::rasterize") {
  GlobeRectangle rect{0.0, 0.0, 1.0, 1.0};
  VectorRasterizer rasterizer(
      std::vector<CartographicPolygon>{
          CartographicPolygon(std::vector<glm::dvec2>{
              glm::dvec2(0.25, 0.25),
              glm::dvec2(0.5, 0.75),
              glm::dvec2(0.75, 0.25)})},
      std::vector<std::array<std::byte, 4>>{std::array<std::byte, 4>{
          std::byte{255},
          std::byte{0},
          std::byte{255},
          std::byte{255}}});

  CesiumGltf::ImageAsset asset;
  asset.width = 256;
  asset.height = 256;
  asset.channels = 4;
  asset.bytesPerChannel = 1;
  asset.pixelData.resize(
      (size_t)(asset.width * asset.height * asset.channels * asset.bytesPerChannel),
      std::byte{0});

  rasterizer.rasterize(rect, asset);
  asset.writeTga("out.tga");
}