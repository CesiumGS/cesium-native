#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumNativeTests/readFile.h"
#include "CesiumUtility/Math.h"

#include <CesiumVectorData/VectorDocument.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <variant>

using namespace CesiumVectorData;
using namespace CesiumUtility;
using namespace CesiumGeospatial;

namespace {
static std::span<const std::byte> stringToBytes(const std::string& str) {
  return std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
}

template <typename T>
void expectParserResult(const std::string& json, const T& expectedResult) {
  Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(json));
  CHECK(!doc.errors.hasErrors());
  REQUIRE(doc.value);
  const VectorNode& root = doc.value->getRootNode();
  REQUIRE(expectedResult == root);
}
} // namespace

TEST_CASE("Parse Point primitives") {
  SUBCASE("Valid points") {
    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [100.0, 0.0]
        }
        )==",
        Point{Cartographic{
            Math::degreesToRadians(100.0),
            Math::degreesToRadians(0.0)}});

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-100.0, 20.0, 500.0]
        }
        )==",
        Point{Cartographic{
            Math::degreesToRadians(-100.0),
            Math::degreesToRadians(20.0),
            500.0}});

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-90, 180.0, -500.0],
            "bbox": [90, -90.0, -50, 30.0, 35.0, 50.0]
        }
        )==",
        Point{
            Cartographic{
                Math::degreesToRadians(-90.0),
                Math::degreesToRadians(180.0),
                -500.0},
            BoundingRegion(
                GlobeRectangle(
                    Math::degreesToRadians(90.0),
                    Math::degreesToRadians(-90.0),
                    Math::degreesToRadians(30.0),
                    Math::degreesToRadians(35.0)),
                -50,
                50)});
  }

  SUBCASE("'coordinates' must exist'") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "'coordinates' member required.");
  }

  SUBCASE("Position must be an array") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point", "coordinates": 2 })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Position value must be 2D or 3D") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point", "coordinates": [2.0] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Position value must be an array with two or three members.");

    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Point", "coordinates": [2.0, 1.0, 0.0, 3.0] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Position value must be an array with two or three members.");
  }

  SUBCASE("Position value must only contain numbers") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Point", "coordinates": [2.0, false] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Position value must be an array of only numbers.");
  }
}

TEST_CASE("Parse MultiPoint primitives") {
  SUBCASE("Valid MultiPoint") {
    expectParserResult(
        R"==(
        {
          "type": "MultiPoint",
          "coordinates": [
            [-75.1428517, 39.9644934, 400],
            [129.6869721, 62.0256947, 100]
          ],
          "bbox": [30.0, -30.0, 40.0, -40.0]
        }
        )==",
        MultiPoint{
            std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100)},
            BoundingRegion(
                GlobeRectangle(
                    Math::degreesToRadians(30.0),
                    Math::degreesToRadians(-30.0),
                    Math::degreesToRadians(40.0),
                    Math::degreesToRadians(-40.0)),
                0,
                0)});
    expectParserResult(
        R"==(
        {
          "type": "MultiPoint",
          "coordinates": [
            [-75.1428517, 39.9644934, 400],
            [129.6869721, 62.0256947, 100]
          ],
          "exampleA": 40,
          "exampleB": "test"
        }
        )==",
        MultiPoint{
            std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100)},
            std::nullopt,
            std::map<std::string, JsonValue>{
                {"exampleA", JsonValue(40)},
                {"exampleB", JsonValue("test")}}});
  }

  SUBCASE("Coordinates must be an array") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "MultiPoint", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "MultiPoint 'coordinates' member must be an array of positions.");
  }
}

TEST_CASE("Parse LineString primitives") {
  SUBCASE("Valid LineString") {
    expectParserResult(
        R"==(
        {
          "type": "LineString",
          "coordinates": [
            [-75.1428517, 39.9644934, 400],
            [129.6869721, 62.0256947, 100]
          ],
          "bbox": [30.0, -30.0, 40.0, -40.0]
        }
        )==",
        LineString{
            std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100),
            },
            BoundingRegion(
                GlobeRectangle(
                    Math::degreesToRadians(30.0),
                    Math::degreesToRadians(-30.0),
                    Math::degreesToRadians(40.0),
                    Math::degreesToRadians(-40.0)),
                0,
                0)});
  }

  SUBCASE("Coordinates must be an array") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "LineString", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "LineString 'coordinates' member must be an array of positions.");
  }

  SUBCASE("Coordinates must contain two or more positions") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "LineString", "coordinates": [[0, 1, 2]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "LineString 'coordinates' member must contain two or more positions.");
  }
}

TEST_CASE("Parse MultiLineString primitives") {
  SUBCASE("Valid MultiLineString") {
    expectParserResult(
        R"==(
        {
          "type": "MultiLineString",
          "coordinates": [
            [
              [-75.1428517, 39.9644934, 400],
              [129.6869721, 62.0256947, 100]
            ]
          ]
        }
        )==",
        MultiLineString{
            std::vector<std::vector<Cartographic>>{std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100)}}});
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiLineString", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiLineString 'coordinates' member must be "
                                "an array of position arrays.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiLineString", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiLineString", "coordinates": [[[0, 1, 2]]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "MultiLineString 'coordinates' member must be an "
        "array of arrays of 2 or more positions.");
  }
}

TEST_CASE("Parse Polygon primitives") {
  SUBCASE("Valid Polygon") {
    expectParserResult(
        R"==(
        {
          "type": "Polygon",
          "coordinates": [
            [
              [-75.1428517, 39.9644934, 400],
              [129.6869721, 62.0256947, 100],
              [103.8245805, 1.3043744, 100],
              [-80.1976364, 25.7708431, 400]
            ]
          ]
        }
        )==",
        Polygon{
            std::vector<std::vector<Cartographic>>{std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100),
                Cartographic(
                    Math::degreesToRadians(103.8245805),
                    Math::degreesToRadians(1.3043744),
                    100),
                Cartographic(
                    Math::degreesToRadians(-80.1976364),
                    Math::degreesToRadians(25.7708431),
                    400)}}});
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Polygon", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "Polygon 'coordinates' member must be "
                                "an array of position arrays.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Polygon", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Polygon", "coordinates": [[[0, 1, 2], [1, 2, 3], [4, 3, 5]]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "Polygon 'coordinates' member must be an "
                                "array of arrays of 4 or more positions.");
  }
}

TEST_CASE("Parse MultiPolygon primitives") {
  SUBCASE("Valid MultiPolygon") {
    expectParserResult(
        R"==(
        {
          "type": "MultiPolygon",
          "coordinates": [
            [
              [
                [-75.1428517, 39.9644934, 400],
                [129.6869721, 62.0256947, 100],
                [103.8245805, 1.3043744, 100],
                [-80.1976364, 25.7708431, 400]
              ]
            ]
          ]
        }
        )==",
        MultiPolygon{std::vector<std::vector<std::vector<Cartographic>>>{
            std::vector<std::vector<Cartographic>>{std::vector<Cartographic>{
                Cartographic(
                    Math::degreesToRadians(-75.1428517),
                    Math::degreesToRadians(39.9644934),
                    400),
                Cartographic(
                    Math::degreesToRadians(129.6869721),
                    Math::degreesToRadians(62.0256947),
                    100),
                Cartographic(
                    Math::degreesToRadians(103.8245805),
                    Math::degreesToRadians(1.3043744),
                    100),
                Cartographic(
                    Math::degreesToRadians(-80.1976364),
                    Math::degreesToRadians(25.7708431),
                    400)}}}});
  }

  SUBCASE("Coordinates must be an array of arrays of arrays") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of arrays of position arrays.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [1, 2, 3] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of arrays of position arrays.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of position arrays.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [[[1, 2, 3]]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [[[[0, 1, 2], [1, 2, 3], [4, 3, 5]] ]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of arrays of 4 or more positions.");
  }
}

TEST_CASE("Parsing GeometryCollection") {
  SUBCASE("Valid GeometryCollection") {
    expectParserResult(
        R"==(
        {
          "type": "GeometryCollection",
          "geometries": [
            { "type": "Point", "coordinates": [1, 2], "bbox": [40.0, 40.0, -40.0, -40.0] },
            { "type": "LineString", "coordinates": [[1, 2], [3, 4]], "test": 104.0, "test2": false }
          ]
        }  
        )==",
        GeometryCollection{std::vector<GeometryPrimitive>{
            Point{
                Cartographic::fromDegrees(1, 2),
                BoundingRegion(
                    GlobeRectangle(
                        Math::degreesToRadians(40.0),
                        Math::degreesToRadians(40.0),
                        Math::degreesToRadians(-40.0),
                        Math::degreesToRadians(-40.0)),
                    0,
                    0)},
            LineString{
                std::vector<Cartographic>{
                    Cartographic::fromDegrees(1, 2),
                    Cartographic::fromDegrees(3, 4)},
                std::nullopt,
                JsonValue::Object{
                    {"test", JsonValue(104.0)},
                    {"test2", JsonValue(false)}}}}});
  }

  SUBCASE("Requires 'geometries'") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "GeometryCollection" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeometryCollection requires array 'geometries' member.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "GeometryCollection", "geometries": {} })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeometryCollection requires array 'geometries' member.");
  }

  SUBCASE("'geometries' must only include geometry primitives") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "GeometryCollection", "geometries": [{"type": "Feature", "geometry": null, "properties": null}] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Expected geometry, found GeoJSON object Feature.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "GeometryCollection", "geometries": [1, 2, 3] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "GeometryCollection 'geometries' member must "
                                "contain only GeoJSON objects.");
  }
}

TEST_CASE("Parsing Feature") {
  SUBCASE("Valid Feature") {
    expectParserResult(
        R"==(
        {
          "type": "Feature",
          "id": 20,
          "properties": {
            "a": 1,
            "b": false,
            "c": "3"
          },
          "geometry": {
            "type": "LineString",
            "coordinates": [[1,2,3],[4,5,6]]
          },
          "test": "test"
        }
        )==",
        Feature{
            20,
            LineString{std::vector<Cartographic>{
                Cartographic::fromDegrees(1, 2, 3),
                Cartographic::fromDegrees(4, 5, 6)}},
            JsonValue::Object{
                {"a", JsonValue(1)},
                {"b", JsonValue(false)},
                {"c", JsonValue("3")}},
            std::nullopt,
            JsonValue::Object{{"test", JsonValue("test")}}});
  }

  SUBCASE("Missing required members") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Feature" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Feature must have 'geometry' member.");
    doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Feature", "geometry": null })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Feature must have 'properties' member.");
  }

  SUBCASE("'id' must be string or number") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Feature", "id": null })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Feature 'id' member must be either a string or a number.");
  }
}

TEST_CASE("Parsing FeatureCollection") {
  SUBCASE("Valid FeatureCollection") {
    expectParserResult(
        R"==(
        {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "properties": null,
              "geometry": {
                "type": "Point",
                "coordinates": [1, 2, 3]
              }
            }
          ]
        }  
        )==",
        FeatureCollection{std::vector<Feature>{Feature{
            std::monostate(),
            Point{Cartographic::fromDegrees(1, 2, 3)},
            std::nullopt}}});
  }

  SUBCASE("'features' member must be an array of features") {
    Result<VectorDocument> doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "FeatureCollection" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "FeatureCollection must have 'features' member.");
    doc = VectorDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "FeatureCollection", "features": 1 })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "FeatureCollection 'features' member must be an array of features.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "FeatureCollection", "features": [1] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "FeatureCollection 'features' member must "
                                "contain only GeoJSON objects.");
    doc = VectorDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "FeatureCollection", "features": [{"type": "Point", "coordinates": [1,2,3]}] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Expected Feature, found GeoJSON object Point.");
  }
}

TEST_CASE("Load test GeoJSON without errors") {
  std::filesystem::path dir(
      std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
  for (auto& file : std::filesystem::directory_iterator(dir)) {
    if (!file.path().extension().string().ends_with("json")) {
      continue;
    }
    Result<VectorDocument> doc =
        VectorDocument::fromGeoJson(readFile(file.path()));
    CHECK(doc.value);
    CHECK(!doc.errors.hasErrors());
    CHECK(doc.errors.warnings.empty());
  }
}