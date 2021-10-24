#include "Cesium3DTiles/TilesetReader.h"

#include <Cesium3DTiles/Extension3dTilesContentGltf.h>
#include <CesiumJsonReader/JsonReader.h>

#include <catch2/catch.hpp>
#include <glm/vec3.hpp>
#include <gsl/span>
#include <rapidjson/reader.h>

#include <filesystem>
#include <fstream>
#include <string>

using namespace Cesium3DTiles;
using namespace CesiumUtility;

namespace {
std::vector<std::byte> readFile(const std::filesystem::path& fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  REQUIRE(file);

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);

  return buffer;
}
} // namespace

TEST_CASE("Cesium3DTiles::TilesetReader") {
  using namespace std::string_literals;

  std::filesystem::path tilesetFile = Cesium3DTilesReader_TEST_DATA_DIR;
  tilesetFile /= "tileset.json";
  std::vector<std::byte> data = readFile(tilesetFile);
  Cesium3DTiles::TilesetReader reader;
  TilesetReaderResult result = reader.readTileset(data);
  REQUIRE(result.tileset);

  const Tileset& tileset = result.tileset.value();

  REQUIRE(tileset.asset.version == "1.0");
  REQUIRE(tileset.geometricError == 494.50961650991815);
  REQUIRE(tileset.extensionsUsed.size() == 0);
  REQUIRE(tileset.extensionsRequired.size() == 0);

  REQUIRE(tileset.properties.size() == 3);
  CHECK(tileset.properties.at("Longitude").minimum == -0.0005589940528287436);
  CHECK(tileset.properties.at("Longitude").maximum == 0.0001096066770252439);
  CHECK(tileset.properties.at("Latitude").minimum == 0.8987242766850329);
  CHECK(tileset.properties.at("Latitude").maximum == 0.899060112939701);
  CHECK(tileset.properties.at("Height").minimum == 1.0);
  CHECK(tileset.properties.at("Height").maximum == 241.6);

  CHECK(tileset.root.content->uri == "0/0/0.b3dm");
  CHECK(tileset.root.geometricError == 268.37878244706053);
  CHECK(tileset.root.refine == Tile::Refine::ADD);
  CHECK_FALSE(tileset.root.viewerRequestVolume);

  std::vector<double> expectedRegion = {
      -0.0005682966577418737,
      0.8987233516605286,
      0.00011646582098558159,
      0.8990603398325034,
      0,
      241.6};

  std::vector<double> expectedContentRegion = {
      -0.0004001690908972599,
      0.8988700116775743,
      0.00010096729722787196,
      0.8989625664878067,
      0,
      241.6};

  std::vector<double> expectedChildRegion = {
      -0.0004853062518095434,
      0.898741188925484,
      -0.0002736676267127107,
      0.8989037314387226,
      0,
      158.4};

  std::vector<double> expectedChildContentRegion = {
      -0.0004058588642587614,
      0.898746512179703,
      -0.0002736676267127107,
      0.8989037314387226,
      0,
      158.4};

  REQUIRE_THAT(
      tileset.root.boundingVolume.region,
      Catch::Matchers::Approx(expectedRegion));

  REQUIRE_THAT(
      tileset.root.content->boundingVolume->region,
      Catch::Matchers::Approx(expectedContentRegion));

  REQUIRE(tileset.root.children.size() == 4);

  const Tile& child = tileset.root.children[0];

  REQUIRE_THAT(
      child.boundingVolume.region,
      Catch::Matchers::Approx(expectedChildRegion));

  REQUIRE_THAT(
      child.content->boundingVolume->region,
      Catch::Matchers::Approx(expectedChildContentRegion));

  CHECK(child.content->uri == "1/0/0.b3dm");
  CHECK(child.geometricError == 159.43385994848);
  CHECK(child.children.size() == 4);
  CHECK_FALSE(child.viewerRequestVolume);
}

TEST_CASE("Can deserialize 3DTILES_content_gltf") {
  std::string s = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "box": [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        },
        "geometricError": 15.0,
        "refine": "ADD",
        "content": {
          "uri": "root.glb"
        }
      },
      "extensionsUsed": ["3DTILES_content_gltf"],
      "extensionsRequired": ["3DTILES_content_gltf"],
      "extensions": {
        "3DTILES_content_gltf": {
          "extensionsUsed": ["KHR_draco_mesh_compression", "KHR_materials_unlit"],
          "extensionsRequired": ["KHR_draco_mesh_compression"]
        }
      }
    }
  )";

  Cesium3DTiles::TilesetReader reader;
  TilesetReaderResult result = reader.readTileset(
      gsl::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));
  CHECK(result.errors.empty());
  CHECK(result.warnings.empty());
  REQUIRE(result.tileset.has_value());

  Tileset& tileset = result.tileset.value();
  CHECK(tileset.asset.version == "1.0");

  const std::vector<std::string> tilesetExtensionUsed{"3DTILES_content_gltf"};
  const std::vector<std::string> tilesetExtensionRequired{
      "3DTILES_content_gltf"};

  const std::vector<std::string> gltfExtensionsUsed{
      "KHR_draco_mesh_compression",
      "KHR_materials_unlit"};
  const std::vector<std::string> gltfExtensionsRequired{
      "KHR_draco_mesh_compression"};

  CHECK(tileset.extensionsUsed == tilesetExtensionUsed);
  CHECK(tileset.extensionsUsed == tilesetExtensionUsed);

  Extension3dTilesContentGltf* contentGltf =
      tileset.getExtension<Extension3dTilesContentGltf>();
  REQUIRE(contentGltf);
  CHECK(contentGltf->extensionsUsed == gltfExtensionsUsed);
  CHECK(contentGltf->extensionsRequired == gltfExtensionsRequired);
}
