#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>

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

void expectParserResult(
    const std::string& json,
    const std::function<void(const IntrusivePointer<GeoJsonDocument>&)>&
        checkFunc) {
  Result<IntrusivePointer<GeoJsonDocument>> doc =
      GeoJsonDocument::fromGeoJson(stringToBytes(json));
  CHECK(!doc.errors.hasErrors());
  REQUIRE(doc.pValue);
  checkFunc(doc.pValue);
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pDocument->getRootObject());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::Point);
          CHECK(
              pPoint->coordinates ==
              Cartographic::fromDegrees(100.0, 0.0, 0.0));
        });

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-100.0, 20.0, 500.0]
        }
        )==",
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pDocument->getRootObject());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::Point);
          CHECK(
              pPoint->coordinates ==
              Cartographic::fromDegrees(-100.0, 20.0, 500.0));
        });

    expectParserResult(
        R"==(
        {
            "type": "Point",
            "coordinates": [-90, 180.0, -500.0],
            "bbox": [90, -90.0, -50, 30.0, 35.0, 50.0]
        }
        )==",
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pDocument->getRootObject());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::Point);
          CHECK(
              pPoint->coordinates ==
              Cartographic::fromDegrees(-90.0, 180.0, -500.0));
          const std::optional<BoundingRegion>& bbox = pPoint->boundingBox;
          CHECK(
              bbox->getRectangle().getSouthwest() ==
              Cartographic(
                  Math::degreesToRadians(90.0),
                  Math::degreesToRadians(-90.0)));
          CHECK(
              bbox->getRectangle().getNortheast() ==
              Cartographic(
                  Math::degreesToRadians(30.0),
                  Math::degreesToRadians(35.0)));
          CHECK(bbox->getMinimumHeight() == -50.0);
          CHECK(bbox->getMaximumHeight() == 50.0);
        });
  }

  SUBCASE("'coordinates' must exist'") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
            stringToBytes(R"==({ "type": "Point" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "'coordinates' member required.");
  }

  SUBCASE("Position must be an array") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
            stringToBytes(R"==({ "type": "Point", "coordinates": 2 })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Position value must be an array.");
  }

  SUBCASE("Position value must be 2D or 3D") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonMultiPoint* pPoint =
              std::get_if<GeoJsonMultiPoint>(&pDocument->getRootObject());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::MultiPoint);
          REQUIRE(pPoint->coordinates.size() == 2);
          CHECK(
              pPoint->coordinates[0] ==
              Cartographic::fromDegrees(-75.1428517, 39.9644934, 400.0));
          CHECK(
              pPoint->coordinates[1] ==
              Cartographic::fromDegrees(129.6869721, 62.0256947, 100.0));
          const std::optional<BoundingRegion>& bbox = pPoint->boundingBox;
          CHECK(
              bbox->getRectangle().getSouthwest() ==
              Cartographic(
                  Math::degreesToRadians(30.0),
                  Math::degreesToRadians(-30.0)));
          CHECK(
              bbox->getRectangle().getNortheast() ==
              Cartographic(
                  Math::degreesToRadians(40.0),
                  Math::degreesToRadians(-40.0)));
          CHECK(bbox->getMinimumHeight() == 0.0);
          CHECK(bbox->getMaximumHeight() == 0.0);
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonMultiPoint* pPoint =
              std::get_if<GeoJsonMultiPoint>(&pDocument->getRootObject());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::MultiPoint);
          REQUIRE(pPoint->coordinates.size() == 2);
          CHECK(
              pPoint->coordinates[0] ==
              Cartographic::fromDegrees(-75.1428517, 39.9644934, 400.0));
          CHECK(
              pPoint->coordinates[1] ==
              Cartographic::fromDegrees(129.6869721, 62.0256947, 100.0));
          JsonValue::Object foreignMembers = pPoint->foreignMembers;
          REQUIRE(!foreignMembers.empty());
          CHECK(foreignMembers["exampleA"] == JsonValue(40));
          CHECK(foreignMembers["exampleB"] == JsonValue("test"));
        });
  }

  SUBCASE("Coordinates must be an array") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
            R"==({ "type": "MultiPoint", "coordinates": false })=="));
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonLineString* pLine =
              std::get_if<GeoJsonLineString>(&pDocument->getRootObject());
          REQUIRE(pLine);
          REQUIRE(pLine->type == GeoJsonObjectType::LineString);
          const std::vector<Cartographic>& points = pLine->coordinates;
          REQUIRE(points.size() == 2);
          CHECK(
              points[0] ==
              Cartographic::fromDegrees(-75.1428517, 39.9644934, 400.0));
          CHECK(
              points[1] ==
              Cartographic::fromDegrees(129.6869721, 62.0256947, 100.0));
          const std::optional<BoundingRegion>& bbox = pLine->boundingBox;
          CHECK(
              bbox->getRectangle().getSouthwest() ==
              Cartographic(
                  Math::degreesToRadians(30.0),
                  Math::degreesToRadians(-30.0)));
          CHECK(
              bbox->getRectangle().getNortheast() ==
              Cartographic(
                  Math::degreesToRadians(40.0),
                  Math::degreesToRadians(-40.0)));
          CHECK(bbox->getMinimumHeight() == 0.0);
          CHECK(bbox->getMaximumHeight() == 0.0);
        });
  }

  SUBCASE("Coordinates must be an array") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
            R"==({ "type": "LineString", "coordinates": false })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(
        doc.errors.errors[0] ==
        "LineString 'coordinates' member must be an array of positions.");
  }

  SUBCASE("Coordinates must contain two or more positions") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonMultiLineString* pLine =
              std::get_if<GeoJsonMultiLineString>(&pDocument->getRootObject());
          REQUIRE(pLine);
          REQUIRE(pLine->type == GeoJsonObjectType::MultiLineString);
          REQUIRE(pLine->coordinates.size() == 1);
          const std::vector<Cartographic>& points = pLine->coordinates[0];
          REQUIRE(points.size() == 2);
          CHECK(
              points[0] ==
              Cartographic::fromDegrees(-75.1428517, 39.9644934, 400.0));
          CHECK(
              points[1] ==
              Cartographic::fromDegrees(129.6869721, 62.0256947, 100.0));
        });
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonPolygon* pPolygon =
              std::get_if<GeoJsonPolygon>(&pDocument->getRootObject());
          REQUIRE(pPolygon);
          REQUIRE(pPolygon->type == GeoJsonObjectType::Polygon);
          REQUIRE(pPolygon->coordinates.size() == 1);
          const std::vector<Cartographic>& points = pPolygon->coordinates[0];
          REQUIRE(points.size() == 5);
          CHECK(points[0].longitude == Math::degreesToRadians(-75.1428517));
          CHECK(points[0].latitude == Math::degreesToRadians(39.9644934));
          CHECK(points[0].height == 400.0);
          CHECK(points[1].longitude == Math::degreesToRadians(129.6869721));
          CHECK(points[1].latitude == Math::degreesToRadians(62.0256947));
          CHECK(points[1].height == 100.0);
          CHECK(points[2].longitude == Math::degreesToRadians(103.8245805));
          CHECK(points[2].latitude == Math::degreesToRadians(1.3043744));
          CHECK(points[2].height == 100.0);
          CHECK(points[3].longitude == Math::degreesToRadians(-80.1976364));
          CHECK(points[3].latitude == Math::degreesToRadians(25.7708431));
          CHECK(points[3].height == 400.0);
          CHECK(points[4].longitude == Math::degreesToRadians(-75.1428517));
          CHECK(points[4].latitude == Math::degreesToRadians(39.9644934));
          CHECK(points[4].height == 400.0);
        });
  }

  SUBCASE("Coordinates must be an array of arrays") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
            R"==({ "type": "Polygon", "coordinates": false })=="));
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonMultiPolygon* pPolygon =
              std::get_if<GeoJsonMultiPolygon>(&pDocument->getRootObject());
          REQUIRE(pPolygon);
          REQUIRE(pPolygon->type == GeoJsonObjectType::MultiPolygon);
          REQUIRE(pPolygon->coordinates.size() == 1);
          REQUIRE(pPolygon->coordinates[0].size() == 1);
          const std::vector<Cartographic>& points = pPolygon->coordinates[0][0];
          REQUIRE(points.size() == 5);
          CHECK(points[0].longitude == Math::degreesToRadians(-75.1428517));
          CHECK(points[0].latitude == Math::degreesToRadians(39.9644934));
          CHECK(points[0].height == 400.0);
          CHECK(points[1].longitude == Math::degreesToRadians(129.6869721));
          CHECK(points[1].latitude == Math::degreesToRadians(62.0256947));
          CHECK(points[1].height == 100.0);
          CHECK(points[2].longitude == Math::degreesToRadians(103.8245805));
          CHECK(points[2].latitude == Math::degreesToRadians(1.3043744));
          CHECK(points[2].height == 100.0);
          CHECK(points[3].longitude == Math::degreesToRadians(-80.1976364));
          CHECK(points[3].latitude == Math::degreesToRadians(25.7708431));
          CHECK(points[3].height == 400.0);
          CHECK(points[4].longitude == Math::degreesToRadians(-75.1428517));
          CHECK(points[4].latitude == Math::degreesToRadians(39.9644934));
          CHECK(points[4].height == 400.0);
        });
  }

  SUBCASE("Coordinates must be an array of arrays of arrays") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonGeometryCollection* pGeomCollection =
              std::get_if<GeoJsonGeometryCollection>(
                  &pDocument->getRootObject());
          REQUIRE(pGeomCollection);
          REQUIRE(
              pGeomCollection->type == GeoJsonObjectType::GeometryCollection);
          REQUIRE(pGeomCollection->geometries.size() == 2);
          const GeoJsonPoint* pPoint =
              std::get_if<GeoJsonPoint>(&pGeomCollection->geometries[0]);
          REQUIRE(pPoint);
          CHECK(pPoint->type == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == Cartographic::fromDegrees(1, 2));
          const GeoJsonLineString* pLineString =
              std::get_if<GeoJsonLineString>(&pGeomCollection->geometries[1]);
          REQUIRE(pLineString);
          CHECK(pLineString->type == GeoJsonObjectType::LineString);
          const std::vector<Cartographic>& linePoints =
              pLineString->coordinates;
          REQUIRE(linePoints.size() == 2);
          CHECK(linePoints[0] == Cartographic::fromDegrees(1, 2));
          CHECK(linePoints[1] == Cartographic::fromDegrees(3, 4));
          JsonValue::Object foreignMembersObj = pLineString->foreignMembers;
          REQUIRE(!foreignMembersObj.empty());
          CHECK(foreignMembersObj["test"] == JsonValue(104.0));
          CHECK(foreignMembersObj["test2"] == JsonValue(false));
        });
  }

  SUBCASE("Requires 'geometries'") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(stringToBytes(
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
        [](const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonFeature* pFeature =
              std::get_if<GeoJsonFeature>(&pDocument->getRootObject());
          REQUIRE(pFeature);
          REQUIRE(pFeature->type == GeoJsonObjectType::Feature);
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
              std::get_if<GeoJsonLineString>(&pFeature->geometry.value());
          REQUIRE(pLineString);
          REQUIRE(pLineString->type == GeoJsonObjectType::LineString);
          const std::vector<Cartographic>& points = pLineString->coordinates;
          REQUIRE(points.size() == 2);
          CHECK(points[0] == Cartographic::fromDegrees(1, 2, 3));
          CHECK(points[1] == Cartographic::fromDegrees(4, 5, 6));
          JsonValue::Object foreignMembers = pFeature->foreignMembers;
          REQUIRE(!foreignMembers.empty());
          CHECK(foreignMembers["test"] == JsonValue("test"));
        });
  }

  SUBCASE("Missing required members") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
            stringToBytes(R"==({ "type": "Feature" })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Feature must have 'geometry' member.");
    doc = GeoJsonDocument::fromGeoJson(
        stringToBytes(R"==({ "type": "Feature", "geometry": null })=="));
    REQUIRE(doc.errors.hasErrors());
    CHECK(doc.errors.errors.size() == 1);
    CHECK(doc.errors.errors[0] == "Feature must have 'properties' member.");
  }

  SUBCASE("'id' must be string or number") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
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
        [](const IntrusivePointer<GeoJsonDocument>& pDocument) {
          const GeoJsonFeatureCollection* pFeatureCollection =
              std::get_if<GeoJsonFeatureCollection>(
                  &pDocument->getRootObject());
          REQUIRE(pFeatureCollection);
          REQUIRE(
              pFeatureCollection->type == GeoJsonObjectType::FeatureCollection);
          REQUIRE(pFeatureCollection->features.size() == 1);
          CHECK(pFeatureCollection->features[0].properties == std::nullopt);
          const GeoJsonPoint* pPoint = std::get_if<GeoJsonPoint>(
              &pFeatureCollection->features[0].geometry.value());
          REQUIRE(pPoint);
          REQUIRE(pPoint->type == GeoJsonObjectType::Point);
          CHECK(pPoint->coordinates == Cartographic::fromDegrees(1, 2, 3));
        });
  }

  SUBCASE("'features' member must be an array of features") {
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(
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
    Result<IntrusivePointer<GeoJsonDocument>> doc =
        GeoJsonDocument::fromGeoJson(readFile(file.path()));
    CHECK(doc.pValue);
    CHECK(!doc.errors.hasErrors());
    CHECK(doc.errors.warnings.empty());
  }
}