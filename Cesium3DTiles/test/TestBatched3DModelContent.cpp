#include "Batched3DModelContent.h"
#include "catch2/catch.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>
#include "readFile.h"

using namespace Cesium3DTiles;

TEST_CASE("Converts batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath = testFilePath / "Tileset" / "ll.b3dm";
  std::vector<std::byte> b3dm = readFile(testFilePath);

  std::unique_ptr<TileContentLoadResult> pResult =
      Batched3DModelContent::load(spdlog::default_logger(), "test.url", b3dm);

  REQUIRE(pResult->model);
}
