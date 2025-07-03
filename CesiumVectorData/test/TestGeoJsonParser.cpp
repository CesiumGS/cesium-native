#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <variant>

using namespace CesiumVectorData;
using namespace CesiumUtility;
using namespace CesiumGeometry;
using namespace CesiumNativeTests;

namespace {
static std::span<const std::byte> stringToBytes(const std::string& str) {
  return std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
}

void expectParserResult(
    const std::string& json,
    const std::function<void(const GeoJsonDocument&)>& checkFunc) {
  Result<GeoJsonDocument> doc =
      GeoJsonDocument::fromGeoJson(stringToBytes(json));
  CHECK(!doc.errors.hasErrors());
  REQUIRE(doc.value);
  checkFunc(*doc.value);
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
        [](const GeoJsonDocument& document) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&document.rootObject.value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == glm::dvec3(100.0, 0.0, 0.0));
        });

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-100.0, 20.0, 500.0]
        }
        )==",
        [](const GeoJsonDocument& document) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&document.rootObject.value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == glm::dvec3(-100.0, 20.0, 500.0));
        });

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-90, 180.0, -500.0],
            "bbox": [30.0, 35.0, 50.0, 90, -90.0, -50]
        }
        )==",
        [](const GeoJsonDocument& document) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&document.rootObject.value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == glm::dvec3(-90.0, 180.0, -500.0));
          const std::optional<AxisAlignedBox>& bbox = pPoint->boundingBox;
          REQUIRE(bbox);
          CHECK(bbox->minimumX == 30.0);
          CHECK(bbox->minimumY == -90.0);
          CHECK(bbox->maximumX == 90.0);
          CHECK(bbox->maximumY == 35.0);
          CHECK(bbox->minimumZ == -50.0);
          CHECK(bbox->maximumZ == 50.0);
        });
  }

  SUBCASE("'coordinates' must exist'") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "'coordinates' member required.");
  }

  SUBCASE("Position must be an array") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point", "coordinates": 2 })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Position value must be 2D or 3D") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Point", "coordinates": [2.0] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Position value must be an array with two or three members.");

    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Point", "coordinates": [2.0, 1.0, 0.0, 3.0] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "Position value must be an array with two or three members.");
  }

  SUBCASE("Position value must only contain numbers") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonMultiPoint* pPoint =
              std::get_if<GeoJsonMultiPoint>(&document.rootObject.value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::MultiPoint);
          REQUIRE(pPoint->coordinates.size() == 2);
          CHECK(
              pPoint->coordinates[0] ==
              glm::dvec3(-75.1428517, 39.9644934, 400.0));
          CHECK(
              pPoint->coordinates[1] ==
              glm::dvec3(129.6869721, 62.0256947, 100.0));
          const std::optional<AxisAlignedBox>& bbox = pPoint->boundingBox;
          REQUIRE(bbox);
          CHECK(bbox->minimumX == 30.0);
          CHECK(bbox->minimumY == -40.0);
          CHECK(bbox->maximumX == 40.0);
          CHECK(bbox->maximumY == -30.0);
          CHECK(bbox->minimumZ == 0.0);
          CHECK(bbox->maximumZ == 0.0);
        });
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
        [](const GeoJsonDocument& document) {
          const GeoJsonMultiPoint* pPoint =
              std::get_if<GeoJsonMultiPoint>(&document.rootObject.value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::MultiPoint);
          REQUIRE(pPoint->coordinates.size() == 2);
          CHECK(
              pPoint->coordinates[0] ==
              glm::dvec3(-75.1428517, 39.9644934, 400.0));
          CHECK(
              pPoint->coordinates[1] ==
              glm::dvec3(129.6869721, 62.0256947, 100.0));
          JsonValue::Object foreignMembers = pPoint->foreignMembers;
          REQUIRE(!foreignMembers.empty());
          CHECK(foreignMembers["exampleA"] == JsonValue(40));
          CHECK(foreignMembers["exampleB"] == JsonValue("test"));
        });
  }

  SUBCASE("Coordinates must be an array") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonLineString* pLine =
              std::get_if<GeoJsonLineString>(&document.rootObject.value);
          REQUIRE(pLine);
          REQUIRE(pLine->TYPE == GeoJsonObjectType::LineString);
          const std::vector<glm::dvec3>& points = pLine->coordinates;
          REQUIRE(points.size() == 2);
          CHECK(points[0] == glm::dvec3(-75.1428517, 39.9644934, 400.0));
          CHECK(points[1] == glm::dvec3(129.6869721, 62.0256947, 100.0));
          const std::optional<AxisAlignedBox>& bbox = pLine->boundingBox;
          REQUIRE(bbox);
          CHECK(bbox->minimumX == 30.0);
          CHECK(bbox->minimumY == -40.0);
          CHECK(bbox->maximumX == 40.0);
          CHECK(bbox->maximumY == -30.0);
          CHECK(bbox->minimumZ == 0.0);
          CHECK(bbox->maximumZ == 0.0);
        });
  }

  SUBCASE("Coordinates must be an array") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "LineString", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "LineString 'coordinates' member must be an array of positions.");
  }

  SUBCASE("Coordinates must contain two or more positions") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonMultiLineString* pLine =
              std::get_if<GeoJsonMultiLineString>(&document.rootObject.value);
          REQUIRE(pLine);
          REQUIRE(pLine->TYPE == GeoJsonObjectType::MultiLineString);
          REQUIRE(pLine->coordinates.size() == 1);
          const std::vector<glm::dvec3>& points = pLine->coordinates[0];
          REQUIRE(points.size() == 2);
          CHECK(points[0] == glm::dvec3(-75.1428517, 39.9644934, 400.0));
          CHECK(points[1] == glm::dvec3(129.6869721, 62.0256947, 100.0));
        });
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiLineString", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiLineString 'coordinates' member must be "
                                "an array of position arrays.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiLineString", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
              [-80.1976364, 25.7708431, 400],
              [-75.1428517, 39.9644934, 400]
            ]
          ]
        }
        )==",
        [](const GeoJsonDocument& document) {
          const GeoJsonPolygon* pPolygon =
              std::get_if<GeoJsonPolygon>(&document.rootObject.value);
          REQUIRE(pPolygon);
          REQUIRE(pPolygon->TYPE == GeoJsonObjectType::Polygon);
          REQUIRE(pPolygon->coordinates.size() == 1);
          const std::vector<glm::dvec3>& points = pPolygon->coordinates[0];
          REQUIRE(points.size() == 5);
          CHECK(points[0].x == -75.1428517);
          CHECK(points[0].y == 39.9644934);
          CHECK(points[0].z == 400.0);
          CHECK(points[1].x == 129.6869721);
          CHECK(points[1].y == 62.0256947);
          CHECK(points[1].z == 100.0);
          CHECK(points[2].x == 103.8245805);
          CHECK(points[2].y == 1.3043744);
          CHECK(points[2].z == 100.0);
          CHECK(points[3].x == -80.1976364);
          CHECK(points[3].y == 25.7708431);
          CHECK(points[3].z == 400.0);
          CHECK(points[4].x == -75.1428517);
          CHECK(points[4].y == 39.9644934);
          CHECK(points[4].z == 400.0);
        });
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Polygon", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "Polygon 'coordinates' member must be "
                                "an array of position arrays.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "Polygon", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
                [-80.1976364, 25.7708431, 400],
                [-75.1428517, 39.9644934, 400]
              ]
            ]
          ]
        }
        )==",
        [](const GeoJsonDocument& document) {
          const GeoJsonMultiPolygon* pPolygon =
              std::get_if<GeoJsonMultiPolygon>(&document.rootObject.value);
          REQUIRE(pPolygon);
          REQUIRE(pPolygon->TYPE == GeoJsonObjectType::MultiPolygon);
          REQUIRE(pPolygon->coordinates.size() == 1);
          REQUIRE(pPolygon->coordinates[0].size() == 1);
          const std::vector<glm::dvec3>& points = pPolygon->coordinates[0][0];
          REQUIRE(points.size() == 5);
          CHECK(points[0].x == -75.1428517);
          CHECK(points[0].y == 39.9644934);
          CHECK(points[0].z == 400.0);
          CHECK(points[1].x == 129.6869721);
          CHECK(points[1].y == 62.0256947);
          CHECK(points[1].z == 100.0);
          CHECK(points[2].x == 103.8245805);
          CHECK(points[2].y == 1.3043744);
          CHECK(points[2].z == 100.0);
          CHECK(points[3].x == -80.1976364);
          CHECK(points[3].y == 25.7708431);
          CHECK(points[3].z == 400.0);
          CHECK(points[4].x == -75.1428517);
          CHECK(points[4].y == 39.9644934);
          CHECK(points[4].z == 400.0);
        });
  }

  SUBCASE("Coordinates must be an array of arrays of arrays") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of arrays of position arrays.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [1, 2, 3] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of arrays of position arrays.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [[1, 2, 3]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "MultiPolygon 'coordinates' member must be an "
                                "array of position arrays.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "MultiPolygon", "coordinates": [[[1, 2, 3]]] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Lines must contain two or more positions") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonGeometryCollection* pGeomCollection =
              std::get_if<GeoJsonGeometryCollection>(
                  &document.rootObject.value);
          REQUIRE(pGeomCollection);
          REQUIRE(
              pGeomCollection->TYPE == GeoJsonObjectType::GeometryCollection);
          REQUIRE(pGeomCollection->geometries.size() == 2);
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pGeomCollection->geometries[0].value);
          REQUIRE(pPoint);
          CHECK(pPoint->TYPE == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == glm::dvec3(1.0, 2.0, 0.0));
          const GeoJsonLineString* pLineString = std::get_if<GeoJsonLineString>(
              &pGeomCollection->geometries[1].value);
          REQUIRE(pLineString);
          CHECK(pLineString->TYPE == GeoJsonObjectType::LineString);
          const std::vector<glm::dvec3>& linePoints = pLineString->coordinates;
          REQUIRE(linePoints.size() == 2);
          CHECK(linePoints[0] == glm::dvec3(1.0, 2.0, 0.0));
          CHECK(linePoints[1] == glm::dvec3(3.0, 4.0, 0.0));
          JsonValue::Object foreignMembersObj = pLineString->foreignMembers;
          REQUIRE(!foreignMembersObj.empty());
          CHECK(foreignMembersObj["test"] == JsonValue(104.0));
          CHECK(foreignMembersObj["test2"] == JsonValue(false));
        });
  }

  SUBCASE("Requires 'geometries'") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "GeometryCollection" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeometryCollection requires array 'geometries' member.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "GeometryCollection", "geometries": {} })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeometryCollection requires array 'geometries' member.");
  }

  SUBCASE("'geometries' must only include geometry primitives") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "GeometryCollection", "geometries": [{"type": "Feature", "geometry": null, "properties": null}] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeoJSON GeometryCollection 'geometries' member may only contain "
        "GeoJSON Geometry objects, found Feature.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonFeature* pFeature =
              std::get_if<GeoJsonFeature>(&document.rootObject.value);
          REQUIRE(pFeature);
          REQUIRE(pFeature->TYPE == GeoJsonObjectType::Feature);
          REQUIRE(pFeature->geometry);
          const int64_t* pId = std::get_if<int64_t>(&pFeature->id);
          REQUIRE(pId);
          CHECK(*pId == 20);
          CHECK(
              pFeature->properties == JsonValue::Object{
                                          {"a", JsonValue(1)},
                                          {"b", JsonValue(false)},
                                          {"c", JsonValue("3")}});
          const GeoJsonLineString* pLineString =
              std::get_if<GeoJsonLineString>(&pFeature->geometry->value);
          REQUIRE(pLineString);
          REQUIRE(pLineString->TYPE == GeoJsonObjectType::LineString);
          const std::vector<glm::dvec3>& points = pLineString->coordinates;
          REQUIRE(points.size() == 2);
          CHECK(points[0] == glm::dvec3(1.0, 2.0, 3.0));
          CHECK(points[1] == glm::dvec3(4.0, 5.0, 6.0));
          JsonValue::Object foreignMembers = pFeature->foreignMembers;
          REQUIRE(!foreignMembers.empty());
          CHECK(foreignMembers["test"] == JsonValue("test"));
        });
  }

  SUBCASE("Missing required members") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Feature" })=="));
    REQUIRE(!doc.errors.warnings.empty());
    CHECK(doc.errors.warnings.size() == 2);
    CHECK(doc.errors.warnings[0] == "Feature must have a 'geometry' member.");
    CHECK(doc.errors.warnings[1] == "Feature must have a 'properties' member.");
  }

  SUBCASE("'id' must be string or number") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
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
        [](const GeoJsonDocument& document) {
          const GeoJsonFeatureCollection* pFeatureCollection =
              std::get_if<GeoJsonFeatureCollection>(&document.rootObject.value);
          REQUIRE(pFeatureCollection);
          REQUIRE(
              pFeatureCollection->TYPE == GeoJsonObjectType::FeatureCollection);
          REQUIRE(pFeatureCollection->features.size() == 1);
          CHECK(
              pFeatureCollection->features[0]
                  .get<GeoJsonFeature>()
                  .properties == std::nullopt);
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pFeatureCollection->features[0]
                                             .get<GeoJsonFeature>()
                                             .geometry->value);
          REQUIRE(pPoint);
          REQUIRE(pPoint->TYPE == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == glm::dvec3(1, 2, 3));
        });
  }

  SUBCASE("'features' member must be an array of features") {
    Result<GeoJsonDocument> doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "FeatureCollection" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "FeatureCollection must have 'features' member.");
    doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "FeatureCollection", "features": 1 })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "FeatureCollection 'features' member must be an array of features.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "FeatureCollection", "features": [1] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] == "FeatureCollection 'features' member must "
                                "contain only GeoJSON objects.");
    doc = GeoJsonDocument::fromGeoJson(stringToBytes(
        R"==({ "type": "FeatureCollection", "features": [{"type": "Point", "coordinates": [1,2,3]}] })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "GeoJSON FeatureCollection 'features' member may only contain Feature "
        "objects, found Point.");
  }
}

TEST_CASE("Load test GeoJSON without errors") {
  std::filesystem::path dir(
      std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
  for (auto& file : std::filesystem::directory_iterator(dir)) {
    if (!file.path().extension().string().ends_with("json")) {
      continue;
    }
    Result<GeoJsonDocument> doc =
        GeoJsonDocument::fromGeoJson(readFile(file.path()));
    CHECK(doc.value);
    CHECK(!doc.errors.hasErrors());
    CHECK(doc.errors.warnings.empty());
  }
}

TEST_CASE("Load GeoJSON from URL") {
  const std::string url = "http://example.com/point.geojson";
  std::filesystem::path dir(
      std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
      std::make_unique<SimpleAssetResponse>(
          static_cast<uint16_t>(200),
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          readFile(dir / "point.geojson"));
  mockCompletedRequests.insert(
      {url,
       std::make_shared<SimpleAssetRequest>(
           "GET",
           url,
           CesiumAsync::HttpHeaders{},
           std::move(mockCompletedResponse))});

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  CesiumAsync::AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());

  CesiumUtility::Result<GeoJsonDocument> result =
      GeoJsonDocument::fromUrl(asyncSystem, mockAssetAccessor, url)
          .waitInMainThread();

  CHECK(!result.errors.hasErrors());
  REQUIRE(result.value);
  REQUIRE(result.value->rootObject.isType<GeoJsonPoint>());
  CHECK(
      result.value->rootObject.get<GeoJsonPoint>().coordinates ==
      glm::dvec3(42.3, 49.34, 11.3413));
}