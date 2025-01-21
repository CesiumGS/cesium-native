#include <CesiumJsonReader/JsonReader.h>
#include <CesiumQuantizedMeshTerrain/Layer.h>
#include <CesiumQuantizedMeshTerrain/LayerReader.h>
#include <CesiumQuantizedMeshTerrain/LayerWriter.h>

#include <doctest/doctest.h>

#include <vector>

using namespace CesiumJsonReader;
using namespace CesiumQuantizedMeshTerrain;

TEST_CASE("LayerWriter") {
  Layer layer;
  layer.attribution = "attribution";
  layer.bounds = {1.0, 2.0, 3.0, 4.0};
  layer.description = "description";
  layer.format = "format";
  layer.maxzoom = 8;
  layer.metadataAvailability = 10;
  layer.minzoom = 1;
  layer.name = "name";
  layer.parentUrl = "parent";
  layer.projection = "projection";
  layer.scheme = "scheme";
  layer.tiles = {"tiles1", "tiles2"};
  layer.version = "version";

  auto& availableLevel0 = layer.available.emplace_back();

  auto& availableLevel0Item0 = availableLevel0.emplace_back();
  availableLevel0Item0.startX = 0;
  availableLevel0Item0.startY = 0;
  availableLevel0Item0.endX = 1;
  availableLevel0Item0.endY = 1;

  auto& availableLevel0Item1 = availableLevel0.emplace_back();
  availableLevel0Item1.startX = 2;
  availableLevel0Item1.startY = 3;
  availableLevel0Item1.endX = 4;
  availableLevel0Item1.endY = 5;

  auto& availableLevel1 = layer.available.emplace_back();
  auto& availableLevel1Item0 = availableLevel1.emplace_back();
  availableLevel1Item0.startX = 10;
  availableLevel1Item0.startY = 20;
  availableLevel1Item0.endX = 30;
  availableLevel1Item0.endY = 40;

  LayerWriter writer;
  LayerWriterResult writeResult = writer.write(layer, {});

  CHECK(writeResult.errors.empty());
  CHECK(writeResult.warnings.empty());
  CHECK(!writeResult.bytes.empty());

  LayerReader reader;
  ReadJsonResult<Layer> readResult = reader.readFromJson(writeResult.bytes);
  CHECK(readResult.errors.empty());
  CHECK(readResult.warnings.empty());
  REQUIRE(readResult.value);

  Layer& readLayer = *readResult.value;

  CHECK(readLayer.attribution == "attribution");
  CHECK(readLayer.bounds == std::vector{1.0, 2.0, 3.0, 4.0});
  CHECK(readLayer.description == "description");
  CHECK(readLayer.format == "format");
  CHECK(readLayer.maxzoom == 8);
  CHECK(readLayer.metadataAvailability == 10);
  CHECK(readLayer.minzoom == 1);
  CHECK(readLayer.name == "name");
  CHECK(readLayer.parentUrl == "parent");
  CHECK(readLayer.projection == "projection");
  CHECK(readLayer.scheme == "scheme");
  CHECK(readLayer.tiles == std::vector<std::string>{"tiles1", "tiles2"});
  CHECK(readLayer.version == "version");

  CHECK(readLayer.available.size() == 2);
  CHECK(readLayer.available[0].size() == 2);
  CHECK(readLayer.available[0][0].startX == 0);
  CHECK(readLayer.available[0][0].startY == 0);
  CHECK(readLayer.available[0][0].endX == 1);
  CHECK(readLayer.available[0][0].endY == 1);
  CHECK(readLayer.available[0][1].startX == 2);
  CHECK(readLayer.available[0][1].startY == 3);
  CHECK(readLayer.available[0][1].endX == 4);
  CHECK(readLayer.available[0][1].endY == 5);
  CHECK(readLayer.available[1].size() == 1);
  CHECK(readLayer.available[1][0].startX == 10);
  CHECK(readLayer.available[1][0].startY == 20);
  CHECK(readLayer.available[1][0].endX == 30);
  CHECK(readLayer.available[1][0].endY == 40);
}
