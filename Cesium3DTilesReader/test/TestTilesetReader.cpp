#include <Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h>
#include <Cesium3DTilesReader/TilesetReader.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumNativeTests/Comparisons.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

using namespace doctest;

TEST_CASE("Reads tileset JSON") {
  using namespace std::string_literals;

  std::filesystem::path tilesetFile = Cesium3DTilesReader_TEST_DATA_DIR;
  tilesetFile /= "tileset.json";
  std::vector<std::byte> data = readFile(tilesetFile);
  Cesium3DTilesReader::TilesetReader reader;
  auto result = reader.readFromJson(data);
  REQUIRE(result.value);

  const Cesium3DTiles::Tileset& tileset = result.value.value();

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
  CHECK(tileset.root.refine == Cesium3DTiles::Tile::Refine::ADD);
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

  REQUIRE(CesiumNativeTests::compareVectors(
      tileset.root.boundingVolume.region,
      expectedRegion));

  REQUIRE(CesiumNativeTests::compareVectors(
      tileset.root.content->boundingVolume->region,
      expectedContentRegion));

  REQUIRE(tileset.root.children.size() == 4);

  const Cesium3DTiles::Tile& child = tileset.root.children[0];

  REQUIRE(CesiumNativeTests::compareVectors(
      child.boundingVolume.region,
      expectedChildRegion));

  REQUIRE(CesiumNativeTests::compareVectors(
      child.content->boundingVolume->region,
      expectedChildContentRegion));

  CHECK(child.content->uri == "1/0/0.b3dm");
  CHECK(child.geometricError == 159.43385994848);
  CHECK(child.children.size() == 4);
  CHECK_FALSE(child.viewerRequestVolume);
}

TEST_CASE("Reads extras") {
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
        "transform": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0],
        "extras": {
          "D": "Goodbye"
        }
      },
      "extras": {
        "A": "Hello",
        "B": 1234567,
        "C": {
          "C1": {},
          "C2": [1,2,3,4,5],
          "C3": true
        }
      }
    }
  )";

  Cesium3DTilesReader::TilesetReader reader;
  auto result = reader.readFromJson(
      std::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));
  REQUIRE(result.errors.empty());
  REQUIRE(result.warnings.empty());
  REQUIRE(result.value.has_value());

  Cesium3DTiles::Tileset& tileset = result.value.value();

  auto ait = tileset.extras.find("A");
  REQUIRE(ait != tileset.extras.end());
  CHECK(ait->second.isString());
  CHECK(ait->second.getStringOrDefault("") == "Hello");

  auto bit = tileset.extras.find("B");
  REQUIRE(bit != tileset.extras.end());
  CHECK(bit->second.isNumber());
  CHECK(bit->second.getUint64() == 1234567);

  auto cit = tileset.extras.find("C");
  REQUIRE(cit != tileset.extras.end());

  CesiumUtility::JsonValue* pC1 = cit->second.getValuePtrForKey("C1");
  REQUIRE(pC1 != nullptr);
  CHECK(pC1->isObject());
  CHECK(pC1->getObject().empty());

  CesiumUtility::JsonValue* pC2 = cit->second.getValuePtrForKey("C2");
  REQUIRE(pC2 != nullptr);

  CHECK(pC2->isArray());
  CesiumUtility::JsonValue::Array array = pC2->getArray();
  CHECK(array.size() == 5);
  CHECK(array[0].getSafeNumber<double>() == 1.0);
  CHECK(array[1].getSafeNumber<std::uint64_t>() == 2);
  CHECK(array[2].getSafeNumber<std::uint8_t>() == 3);
  CHECK(array[3].getSafeNumber<std::int16_t>() == 4);
  CHECK(array[4].getSafeNumber<std::int32_t>() == 5);

  CesiumUtility::JsonValue* pC3 = cit->second.getValuePtrForKey("C3");
  REQUIRE(pC3 != nullptr);
  CHECK(pC3->isBool());
  CHECK(pC3->getBool());

  auto dit = tileset.root.extras.find("D");
  REQUIRE(dit != tileset.root.extras.end());
  CHECK(dit->second.isString());
  CHECK(dit->second.getStringOrDefault("") == "Goodbye");
}

TEST_CASE("Reads 3DTILES_content_gltf") {
  std::string s = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "geometricError": 45.0,
      "root": {
        "boundingVolume": {
          "extensions": {
            "3DTILES_bounding_volume_S2": {
              "token": "3",
              "minimumHeight": 0,
              "maximumHeight": 1000000
            }
          }
        },
        "geometricError": 15.0,
        "refine": "ADD",
        "content": {
          "uri": "root.glb"
        }
      },
      "extensionsUsed": ["3DTILES_bounding_volume_S2"],
      "extensionsRequired": ["3DTILES_bounding_volume_S2"]
    }
  )";

  Cesium3DTilesReader::TilesetReader reader;
  auto result = reader.readFromJson(
      std::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));
  REQUIRE(result.errors.empty());
  REQUIRE(result.warnings.empty());
  REQUIRE(result.value.has_value());

  Cesium3DTiles::Tileset& tileset = result.value.value();
  CHECK(tileset.asset.version == "1.0");

  const std::vector<std::string> tilesetExtensionUsed{
      "3DTILES_bounding_volume_S2"};
  const std::vector<std::string> tilesetExtensionRequired{
      "3DTILES_bounding_volume_S2"};

  const std::vector<std::string> gltfExtensionsUsed{
      "KHR_draco_mesh_compression",
      "KHR_materials_unlit"};
  const std::vector<std::string> gltfExtensionsRequired{
      "KHR_draco_mesh_compression"};

  CHECK(tileset.extensionsUsed == tilesetExtensionUsed);
  CHECK(tileset.extensionsUsed == tilesetExtensionUsed);

  Cesium3DTiles::Extension3dTilesBoundingVolumeS2* boundingVolumeS2 =
      tileset.root.boundingVolume
          .getExtension<Cesium3DTiles::Extension3dTilesBoundingVolumeS2>();
  REQUIRE(boundingVolumeS2);
  CHECK(boundingVolumeS2->token == "3");
  CHECK(boundingVolumeS2->minimumHeight == 0);
  CHECK(boundingVolumeS2->maximumHeight == 1000000);
}

TEST_CASE("Reads custom extension") {
  std::string s = R"(
    {
      "asset": {
        "version": "1.0"
      },
      "extensions": {
        "A": {
          "test": "Hello"
        },
        "B": {
          "another": "Goodbye"
        }
      }
    }
  )";

  Cesium3DTilesReader::TilesetReader reader;
  auto withCustomExt = reader.readFromJson(
      std::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));
  REQUIRE(withCustomExt.errors.empty());
  REQUIRE(withCustomExt.value.has_value());

  REQUIRE(withCustomExt.value->extensions.size() == 2);

  CesiumUtility::JsonValue* pA = withCustomExt.value->getGenericExtension("A");
  CesiumUtility::JsonValue* pB = withCustomExt.value->getGenericExtension("B");
  REQUIRE(pA != nullptr);
  REQUIRE(pB != nullptr);

  REQUIRE(pA->getValuePtrForKey("test"));
  REQUIRE(pA->getValuePtrForKey("test")->getStringOrDefault("") == "Hello");

  REQUIRE(pB->getValuePtrForKey("another"));
  REQUIRE(
      pB->getValuePtrForKey("another")->getStringOrDefault("") == "Goodbye");

  // Repeat test but this time the extension should be skipped.
  reader.getOptions().setExtensionState(
      "A",
      CesiumJsonReader::ExtensionState::Disabled);
  reader.getOptions().setExtensionState(
      "B",
      CesiumJsonReader::ExtensionState::Disabled);

  auto withoutCustomExt = reader.readFromJson(
      std::span(reinterpret_cast<const std::byte*>(s.c_str()), s.size()));

  auto& zeroExtensions = withoutCustomExt.value->extensions;
  REQUIRE(zeroExtensions.empty());
}

TEST_CASE("Reads tileset JSON with unknown properties") {
  using namespace std::string_literals;

  std::filesystem::path tilesetFile = Cesium3DTilesReader_TEST_DATA_DIR;
  tilesetFile /= "tileset-with-unsupported-properties.json";
  std::vector<std::byte> data = readFile(tilesetFile);
  Cesium3DTilesReader::TilesetReader reader;
  auto result = reader.readFromJson(data);
  CHECK(result.errors.empty());
  CHECK(result.warnings.empty());
  REQUIRE(result.value);

  const CesiumUtility::JsonValue::Object& unknownProperties =
      result.value->asset.unknownProperties;

  auto itString = unknownProperties.find("someString");
  REQUIRE(itString != unknownProperties.end());
  REQUIRE(itString->second.isString());
  REQUIRE(itString->second.getString() == "A");

  auto itDouble = unknownProperties.find("someDouble");
  REQUIRE(itDouble != unknownProperties.end());
  REQUIRE(itDouble->second.isDouble());
  REQUIRE(itDouble->second.getDouble() == 2.1);

  auto itInt = unknownProperties.find("someInt");
  REQUIRE(itInt != unknownProperties.end());
  REQUIRE(itInt->second.isUint64());
  REQUIRE(itInt->second.getUint64() == 5);

  auto itSignedInt = unknownProperties.find("someSignedInt");
  REQUIRE(itSignedInt != unknownProperties.end());
  REQUIRE(itSignedInt->second.isInt64());
  REQUIRE(itSignedInt->second.getInt64() == -5);

  auto itBool = unknownProperties.find("someBool");
  REQUIRE(itBool != unknownProperties.end());
  REQUIRE(itBool->second.isBool());
  REQUIRE(itBool->second.getBool() == true);

  auto itArray = unknownProperties.find("someArray");
  REQUIRE(itArray != unknownProperties.end());
  REQUIRE(itArray->second.isArray());
  const CesiumUtility::JsonValue::Array& array = itArray->second.getArray();
  REQUIRE(array.size() == 1);
  REQUIRE(array[0].isString());
  REQUIRE(array[0].getString() == "hi");

  auto itObject = unknownProperties.find("someObject");
  REQUIRE(itObject != unknownProperties.end());
  REQUIRE(itObject->second.isObject());
  const CesiumUtility::JsonValue::Object& o = itObject->second.getObject();
  REQUIRE(o.size() == 1);
  auto itObjectValue = o.find("value");
  REQUIRE(itObjectValue != o.end());
  REQUIRE(itObjectValue->second.isString());
  REQUIRE(itObjectValue->second.getString() == "test");

  auto itNull = unknownProperties.find("someNull");
  REQUIRE(itNull != unknownProperties.end());
  REQUIRE(itNull->second.isNull());
}

TEST_CASE("Reads tileset JSON with unknown properties and ignores them when "
          "requested") {
  using namespace std::string_literals;

  std::filesystem::path tilesetFile = Cesium3DTilesReader_TEST_DATA_DIR;
  tilesetFile /= "tileset-with-unsupported-properties.json";
  std::vector<std::byte> data = readFile(tilesetFile);
  Cesium3DTilesReader::TilesetReader reader;
  reader.getOptions().setCaptureUnknownProperties(false);
  auto result = reader.readFromJson(data);
  CHECK(result.errors.empty());
  CHECK(result.warnings.empty());
  REQUIRE(result.value);

  const CesiumUtility::JsonValue::Object& unknownProperties =
      result.value->asset.unknownProperties;
  CHECK(unknownProperties.empty());
}
