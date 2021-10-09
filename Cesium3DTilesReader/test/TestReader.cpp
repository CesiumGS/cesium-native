#include "Cesium3DTiles/TilesetReader.h"
#include <CesiumJsonReader/JsonReader.h>
#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <rapidjson/reader.h>
#include <string>

using namespace Cesium3DTiles;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {
// std::vector<std::byte> readFile(const std::string& path) {
//   FILE* fp = std::fopen(path.c_str(), "rb");
//   printf("opening file %s\n", path.c_str());
//   REQUIRE(fp);

//   try {
//     std::fseek(fp, 0, SEEK_END);
//     long pos = std::ftell(fp);
//     std::fseek(fp, 0, SEEK_SET);

//     std::vector<std::byte> result(static_cast<size_t>(pos));
//     size_t itemsRead = std::fread(result.data(), 1, result.size(), fp);
//     REQUIRE(itemsRead == result.size());

//     std::fclose(fp);

//     return result;
//   } catch (...) {
//     if (fp) {
//       std::fclose(fp);
//     }
//     throw;
//   }
// }
} // namespace

TEST_CASE("Cesium3DTiles::TilesetReader") {
  // using namespace std::string_literals;

  // std::tilesetFile::path tilesetFile = Cesium3DTilesReader_TEST_DATA_DIR;
  // tilesetFile /= "tileset.json";
  // std::vector<std::byte> data = readFile(tilesetFile.string());

  // ExtensionReaderContext context;
  // TilesetJsonHandler reader(context);
  // ReadJsonResult<Tileset> jsonResult = JsonReader::readJson(data, reader);

  // REQUIRE(jsonResult.value.has_value());
  // const Tileset& tileset = *jsonResult.value;

  // CHECK(tileset.asset.version == "1.0");
  // CHECK(tileset.geometricError == 494.50961650991815);
  // CHECK_FALSE(tileset.extensionsUsed.has_value());
  // CHECK_FALSE(tileset.extensionsRequired.has_value());

  // REQUIRE(tileset.properties.has_value());
  // CHECK(tileset.properties->additionalProperties.size() == 3);
  // CHECK(
  //     tileset.properties->additionalProperties.at("Longitude").minimum ==
  //     -0.0005589940528287436);
  // CHECK(
  //     tileset.properties->additionalProperties.at("Longitude").maximum ==
  //     0.0001096066770252439);
  // CHECK(
  //     tileset.properties->additionalProperties.at("Latitude").minimum ==
  //     0.8987242766850329);
  // CHECK(
  //     tileset.properties->additionalProperties.at("Latitude").maximum ==
  //     0.899060112939701);
  // CHECK(tileset.properties->additionalProperties.at("Height").minimum
  // == 1.0);
  // CHECK(tileset.properties->additionalProperties.at("Height").maximum ==
  // 241.6);

  // CHECK_THAT(
  //     *tileset.root.boundingVolume.region,
  //     Catch::Matchers::Approx(std::vector<double>{
  //         -0.0005682966577418737,
  //         0.8987233516605286,
  //         0.00011646582098558159,
  //         0.8990603398325034,
  //         0,
  //         241.6}));

  // REQUIRE(tileset.root.children->size() == 4);
  // CHECK_THAT(
  //     *tileset.root.content->boundingVolume->region,
  //     Catch::Matchers::Approx(std::vector<double>{
  //         -0.0004001690908972599,
  //         0.8988700116775743,
  //         0.00010096729722787196,
  //         0.8989625664878067,
  //         0,
  //         241.6}));
  // CHECK(tileset.root.content->uri == "0/0/0.b3dm");
  // CHECK(tileset.root.geometricError == 268.37878244706053);
  // CHECK(tileset.root.refine == Tile::Refine::ADD);
  // CHECK_FALSE(tileset.root.viewerRequestVolume.has_value());

  // const std::vector<Tile>& children = *tileset.root.children;
  // CHECK_THAT(
  //     *children[0].content->boundingVolume->region,
  //     Catch::Matchers::Approx(std::vector<double>{
  //         -0.0004058588642587614,
  //         0.898746512179703,
  //         -0.0002736676267127107,
  //         0.8989037314387226,
  //         0,
  //         158.4}));
  // CHECK(children[0].content->uri == "1/0/0.b3dm");
  // CHECK(children[0].geometricError == 159.43385994848);
  // CHECK(children[0].children->size() == 4);
  // CHECK_FALSE(children[0].viewerRequestVolume.has_value());
}
