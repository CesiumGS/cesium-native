#include "Cesium3DTiles/TilesetWriter.h"
#include <Cesium3DTiles/Tileset.h>
#include <algorithm>
#include <catch2/catch.hpp>
#include <cstddef>
#include <gsl/span>
#include <rapidjson/document.h>
#include <string>

TEST_CASE("Write 3D Tiles Tileset", "[3DTilesWriter]") {
  using namespace std::string_literals;

  Cesium3DTiles::Tile t1, t2, t3;
  t1.boundingVolume.box = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  t1.geometricError = 15.0;
  t1.refine = Cesium3DTiles::Tile::Refine::ADD;

  t2.boundingVolume.box = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
  t2.viewerRequestVolume = Cesium3DTiles::BoundingVolume();
  t2.viewerRequestVolume
      ->box = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
  t2.geometricError = 25.0;
  t2.refine = Cesium3DTiles::Tile::Refine::REPLACE;

  t3.boundingVolume.box = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  t3.geometricError = 35.0;
  t3.children = {t1, t2};

  Cesium3DTiles::Asset asset;
  asset.version = "version";

  Cesium3DTiles::TilesetProperties p1, p2;
  p1.maximum = 10.0;
  p1.minimum = 0.0;
  p2.maximum = 5.0;
  p2.minimum = 1.0;

  Cesium3DTiles::Tileset::Properties ps;
  ps.additionalProperties = {{"p1", p1}, {"p2", p2}};

  Cesium3DTiles::Tileset ts;
  ts.asset = asset;
  ts.root = t3;
  ts.extensionsUsed = {"ext1", "ext2"};
  ts.properties = ps;

  CesiumJsonWriter::JsonWriter jsonWriter;
  writeTileset(ts, jsonWriter);

  rapidjson::Document document;
  document.Parse(jsonWriter.toString().c_str());

  CHECK(document.IsObject());
  CHECK(document.HasMember("asset"));
  CHECK(document.HasMember("root"));
  CHECK(document.HasMember("extensionsUsed"));
  CHECK(document.HasMember("geometricError"));
  CHECK(document.HasMember("properties"));
  CHECK_FALSE(document.HasMember("extensionsRequired"));
  CHECK_FALSE(document.HasMember("extensions"));
  CHECK_FALSE(document.HasMember("extras"));

  CHECK(document["asset"].IsObject());
  CHECK(document["asset"].HasMember("version"));
  CHECK(document["asset"]["version"].GetString() == asset.version);
  CHECK_FALSE(document["asset"].HasMember("tilesetVersion"));
  CHECK_FALSE(document["asset"].HasMember("extensions"));
  CHECK_FALSE(document["asset"].HasMember("extras"));

  CHECK(document["extensionsUsed"].IsArray());
  CHECK(document["extensionsUsed"].Size() == ts.extensionsUsed->size());
  for (unsigned int i = 0; i < ts.extensionsUsed->size(); i++) {
    CHECK(
        document["extensionsUsed"][i].GetString() == ts.extensionsUsed->at(i));
  }

  CHECK(document["properties"].IsObject());
  CHECK(document["properties"]["p1"].IsObject());
  CHECK(document["properties"]["p1"]["maximum"].IsDouble());
  CHECK(document["properties"]["p1"]["maximum"] == p1.maximum);
  CHECK(document["properties"]["p1"]["minimum"].IsDouble());
  CHECK(document["properties"]["p1"]["minimum"] == p1.minimum);
  CHECK(document["properties"]["p2"].IsObject());
  CHECK(document["properties"]["p2"]["maximum"].IsDouble());
  CHECK(document["properties"]["p2"]["maximum"] == p2.maximum);
  CHECK(document["properties"]["p2"]["minimum"].IsDouble());
  CHECK(document["properties"]["p2"]["minimum"] == p2.minimum);

  CHECK(document["root"].IsObject());
  CHECK(document["root"].HasMember("boundingVolume"));
  CHECK(document["root"].HasMember("geometricError"));
  CHECK_FALSE(document["root"].HasMember("viewerRequestVolume"));
  CHECK_FALSE(document["root"].HasMember("refine"));
  CHECK_FALSE(document["root"].HasMember("content"));
  CHECK(document["root"].HasMember("children"));
  CHECK(document["root"]["children"].Size() == t3.children->size());

  const rapidjson::Value& child1 = document["root"]["children"][0];
  CHECK(child1.HasMember("boundingVolume"));
  CHECK(child1.HasMember("geometricError"));
  CHECK(child1.HasMember("refine"));
  CHECK(child1["refine"].IsString());
  CHECK(child1["refine"] == "ADD");
  CHECK_FALSE(child1.HasMember("content"));
  CHECK_FALSE(child1.HasMember("children"));
  CHECK_FALSE(child1.HasMember("viewerRequestVolume"));

  const rapidjson::Value& child2 = document["root"]["children"][1];
  CHECK(child2.HasMember("boundingVolume"));
  CHECK(child2.HasMember("geometricError"));
  CHECK(child2.HasMember("viewerRequestVolume"));
  CHECK(child2.HasMember("refine"));
  CHECK(child2["refine"].IsString());
  CHECK(child2["refine"] == "REPLACE");
  CHECK_FALSE(child2.HasMember("content"));
  CHECK_FALSE(child2.HasMember("children"));
}
