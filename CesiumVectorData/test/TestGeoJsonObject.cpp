#include "CesiumVectorData/GeoJsonObjectTypes.h"

#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <span>
#include <string>

using namespace CesiumVectorData;
using namespace CesiumUtility;
using namespace CesiumGeometry;

namespace {
static std::span<const std::byte> stringToBytes(const std::string& str) {
  return std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
}

GeoJsonObject parseGeoJsonObject(const std::string& json) {
  Result<GeoJsonDocument> doc =
      GeoJsonDocument::fromGeoJson(stringToBytes(json));
  CHECK(!doc.errors.hasErrors());
  REQUIRE(doc.value);
  return doc.value->rootObject;
}

template <typename T>
bool operator==(const std::vector<T>& vectorA, const std::vector<T>& vectorB) {
  if (vectorA.size() != vectorB.size()) {
    return false;
  }

  for (size_t i = 0; i < vectorA.size(); i++) {
    if (vectorA[i] != vectorB[i]) {
      return false;
    }
  }

  return true;
}
} // namespace

TEST_CASE("GeoJsonObject::points") {
  SUBCASE("Point object") {
    GeoJsonObject pointsObj = parseGeoJsonObject(R"==(
    {
      "type": "Point",
      "coordinates": [1, 2, 3] 
    })==");

    std::vector<glm::dvec3> points(
        pointsObj.points().begin(),
        pointsObj.points().end());
    CHECK(points.size() == 1);
    CHECK(points[0] == glm::dvec3(1.0, 2.0, 3.0));
  }

  SUBCASE("MultiPoint object") {
    GeoJsonObject pointsObj = parseGeoJsonObject(R"==(
    {
      "type": "MultiPoint",
      "coordinates": [
        [ 1, 2, 3 ],
        [ 2, 3, 4 ],
        [ 3, 4, 5 ],
        [ 4, 5, 6 ],
        [ 5, 6, 7 ]
      ] 
    })==");

    std::vector<glm::dvec3> points(
        pointsObj.points().begin(),
        pointsObj.points().end());
    CHECK(points.size() == 5);
    CHECK(points[0] == glm::dvec3(1.0, 2.0, 3.0));
    CHECK(points[1] == glm::dvec3(2.0, 3.0, 4.0));
    CHECK(points[2] == glm::dvec3(3.0, 4.0, 5.0));
    CHECK(points[3] == glm::dvec3(4.0, 5.0, 6.0));
    CHECK(points[4] == glm::dvec3(5.0, 6.0, 7.0));
  }

  SUBCASE("GeometryCollection") {
    GeoJsonObject pointsObj = parseGeoJsonObject(R"==(
    {
      "type": "GeometryCollection", 
      "geometries": [
        {
          "type": "Point",
          "coordinates": [ 1, 2, 3 ]
        },
        {
          "type": "GeometryCollection",
          "geometries": [
            {
              "type": "Point",
              "coordinates": [ 2, 3, 4 ]
            },
            {
              "type": "MultiPoint",
              "coordinates": [ 
                [ 3, 4, 5 ],
                [ 4, 5, 6 ]
              ] 
            }
          ] 
        },
        {
          "type": "MultiPoint",
          "coordinates": [ ]
        },
        {
          "type": "Point",
          "coordinates": [ 5, 6, 7 ]
        }
      ]
    })==");

    std::vector<glm::dvec3> points(
        pointsObj.points().begin(),
        pointsObj.points().end());
    CHECK(points.size() == 5);
    CHECK(points[0] == glm::dvec3(1.0, 2.0, 3.0));
    CHECK(points[1] == glm::dvec3(2.0, 3.0, 4.0));
    CHECK(points[2] == glm::dvec3(3.0, 4.0, 5.0));
    CHECK(points[3] == glm::dvec3(4.0, 5.0, 6.0));
    CHECK(points[4] == glm::dvec3(5.0, 6.0, 7.0));
  }
}

TEST_CASE("GeoJsonObject::lines") {
  SUBCASE("LineString object") {
    GeoJsonObject linesObj = parseGeoJsonObject(R"==(
    {
      "type": "LineString",
      "coordinates": [
        [1, 2, 3],
        [4, 5, 6]
      ]
    })==");

    std::vector<std::vector<glm::dvec3>> lines(
        linesObj.lines().begin(),
        linesObj.lines().end());
    const std::vector<std::vector<glm::dvec3>> linesExpected{
        {glm::dvec3(1, 2, 3), glm::dvec3(4, 5, 6)}};
    CHECK(lines == linesExpected);
  }

  SUBCASE("MultiLineString object") {
    GeoJsonObject linesObj = parseGeoJsonObject(R"==(
    {
      "type": "MultiLineString",
      "coordinates": [
        [ 
          [ 1, 2, 3 ],
          [ 2, 3, 4 ] 
        ],
        [ 
          [ 3, 4, 5 ],
          [ 4, 5, 6 ] 
        ],
        [
          [ 5, 6, 7 ],
          [ 6, 7, 8 ],
          [ 7, 8, 9 ]
        ]
      ] 
    })==");

    std::vector<std::vector<glm::dvec3>> lines(
        linesObj.lines().begin(),
        linesObj.lines().end());
    const std::vector<std::vector<glm::dvec3>> linesExpected{
        {glm::dvec3(1, 2, 3), glm::dvec3(2, 3, 4)},
        {glm::dvec3(3, 4, 5), glm::dvec3(4, 5, 6)},
        {glm::dvec3(5, 6, 7), glm::dvec3(6, 7, 8), glm::dvec3(7, 8, 9)}};
    CHECK(lines == linesExpected);
  }

  SUBCASE("Complex") {
    GeoJsonObject linesObj = parseGeoJsonObject(R"==(
    {
      "type": "FeatureCollection",
      "features": [
        {
          "type": "Feature",
          "properties": null,
          "geometry": {
            "type": "GeometryCollection",
            "geometries": [
              {
                "type": "LineString",
                "coordinates": [
                  [ 1, 2, 3 ],
                  [ 4, 5, 6 ]
                ]
              },
              {
                "type": "Point",
                "coordinates": [ 0, 1, 2 ]
              },
              {
                "type": "GeometryCollection",
                "geometries": [
                  {
                    "type": "LineString",
                    "coordinates": [
                      [ 2, 3, 4 ],
                      [ 3, 4, 5 ],
                      [ 4, 5, 6 ]
                    ]
                  },
                  {
                    "type": "MultiLineString",
                    "coordinates": [
                      [ 
                        [ 1, 2, 3 ],
                        [ 4, 5, 6 ],
                        [ 7, 8, 9 ],
                        [ 10, 11, 12 ]
                      ],
                      [ 
                        [ 0, 1, 2 ],
                        [ 1, 2, 3 ],
                        [ 2, 3, 4 ]
                      ]
                    ]
                  }
                ]
              },
              {
                "type": "MultiLineString",
                "coordinates": []
              },
              {
                "type": "LineString",
                "coordinates": [
                  [ 1, 2, 3 ],
                  [ 4, 5, 6 ]
                ]
              }
            ]
          }
        }
      ]
    })==");

    std::vector<std::vector<glm::dvec3>> lines(
        linesObj.lines().begin(),
        linesObj.lines().end());
    const std::vector<std::vector<glm::dvec3>> linesExpected{
        {glm::dvec3(1, 2, 3), glm::dvec3(4, 5, 6)},
        {glm::dvec3(2, 3, 4), glm::dvec3(3, 4, 5), glm::dvec3(4, 5, 6)},
        {glm::dvec3(1, 2, 3),
         glm::dvec3(4, 5, 6),
         glm::dvec3(7, 8, 9),
         glm::dvec3(10, 11, 12)},
        {glm::dvec3(0, 1, 2), glm::dvec3(1, 2, 3), glm::dvec3(2, 3, 4)},
        {glm::dvec3(1, 2, 3), glm::dvec3(4, 5, 6)}};
    CHECK(lines == linesExpected);
  }
}

TEST_CASE("GeoJsonObject::polygons") {
  SUBCASE("Polygon object") {
    GeoJsonObject polyObj = parseGeoJsonObject(R"==(
    {
      "type": "Polygon",
      "coordinates": [
        [ 
          [1, 2, 3],
          [4, 5, 6],
          [5, 6, 7],
          [1, 2, 3]
        ]
      ]
    })==");

    std::vector<std::vector<std::vector<glm::dvec3>>> polygons(
        polyObj.polygons().begin(),
        polyObj.polygons().end());
    const std::vector<std::vector<std::vector<glm::dvec3>>> polygonsExpected{
        {{glm::dvec3(1, 2, 3),
          glm::dvec3(4, 5, 6),
          glm::dvec3(5, 6, 7),
          glm::dvec3(1, 2, 3)}}};
    CHECK(polygons == polygonsExpected);
  }

  SUBCASE("MultiPolygon object") {
    GeoJsonObject polyObj = parseGeoJsonObject(R"==(
    {
      "type": "MultiPolygon",
      "coordinates": [
        [ 
          [ 
            [ 1, 2, 3 ],
            [ 2, 3, 4 ],
            [ 4, 5, 6 ],
            [ 1, 2, 3 ]
          ],
          [ 
            [ 3, 4, 5 ],
            [ 4, 5, 6 ],
            [ 5, 6, 7 ],
            [ 3, 4, 5 ]
          ]
        ],
        [
          [
            [ 5, 6, 7 ],
            [ 6, 7, 8 ],
            [ 7, 8, 9 ],
            [ 5, 6, 7 ]
          ]
        ]
      ] 
    })==");

    std::vector<std::vector<std::vector<glm::dvec3>>> polygons(
        polyObj.polygons().begin(),
        polyObj.polygons().end());
    const std::vector<std::vector<std::vector<glm::dvec3>>> polygonsExpected{
        {{glm::dvec3(1, 2, 3),
          glm::dvec3(2, 3, 4),
          glm::dvec3(4, 5, 6),
          glm::dvec3(1, 2, 3)},
         {glm::dvec3(3, 4, 5),
          glm::dvec3(4, 5, 6),
          glm::dvec3(5, 6, 7),
          glm::dvec3(3, 4, 5)}},
        {{glm::dvec3(5, 6, 7),
          glm::dvec3(6, 7, 8),
          glm::dvec3(7, 8, 9),
          glm::dvec3(5, 6, 7)}}};
    CHECK(polygons == polygonsExpected);
  }

  SUBCASE("Complex") {
    GeoJsonObject polyObj = parseGeoJsonObject(R"==(
    {
      "type": "FeatureCollection",
      "features": [
        {
          "type": "Feature",
          "properties": null,
          "geometry": {
            "type": "GeometryCollection",
            "geometries": [
              {
                "type": "Polygon",
                "coordinates": [
                  [
                    [ 1, 2, 3 ],
                    [ 4, 5, 6 ],
                    [ 5, 6, 7 ],
                    [ 1, 2, 3 ]
                  ]
                ]
              },
              {
                "type": "Point",
                "coordinates": [ 0, 1, 2 ]
              },
              {
                "type": "GeometryCollection",
                "geometries": [
                  {
                    "type": "Polygon",
                    "coordinates": [
                      [
                        [ 2, 3, 4 ],
                        [ 3, 4, 5 ],
                        [ 4, 5, 6 ],
                        [ 2, 3, 4 ]
                      ],
                      [
                        [ 1, 2, 3 ],
                        [ 2, 3, 4 ],
                        [ 3, 4, 5 ],
                        [ 1, 2, 3 ]
                      ]
                    ]
                  },
                  {
                    "type": "MultiPolygon",
                    "coordinates": [
                      [
                        [
                          [ 2, 3, 4 ],
                          [ 3, 4, 5 ],
                          [ 4, 5, 6 ],
                          [ 2, 3, 4 ]
                        ],
                        [
                          [ 1, 2, 3 ],
                          [ 2, 3, 4 ],
                          [ 3, 4, 5 ],
                          [ 1, 2, 3 ]
                        ]
                      ],                 
                      [
                        [
                          [ 1, 2, 3 ],
                          [ 4, 5, 6 ],
                          [ 5, 6, 7 ],
                          [ 1, 2, 3 ]
                        ]
                      ]
                    ]
                  }
                ]
              },
              {
                "type": "MultiPolygon",
                "coordinates": []
              },
              {
                "type": "Polygon",
                "coordinates": [
                  [
                    [ 1, 2, 3 ],
                    [ 4, 5, 6 ],
                    [ 7, 8, 9 ],
                    [ 1, 2, 3 ]
                  ]
                ]
              }
            ]
          }
        }
      ]
    })==");

    std::vector<std::vector<std::vector<glm::dvec3>>> polygons(
        polyObj.polygons().begin(),
        polyObj.polygons().end());
    const std::vector<std::vector<std::vector<glm::dvec3>>> polygonsExpected{
        {{glm::dvec3(1, 2, 3),
          glm::dvec3(4, 5, 6),
          glm::dvec3(5, 6, 7),
          glm::dvec3(1, 2, 3)}},
        {{glm::dvec3(2, 3, 4),
          glm::dvec3(3, 4, 5),
          glm::dvec3(4, 5, 6),
          glm::dvec3(2, 3, 4)},
         {glm::dvec3(1, 2, 3),
          glm::dvec3(2, 3, 4),
          glm::dvec3(3, 4, 5),
          glm::dvec3(1, 2, 3)}},
        {{glm::dvec3(2, 3, 4),
          glm::dvec3(3, 4, 5),
          glm::dvec3(4, 5, 6),
          glm::dvec3(2, 3, 4)},
         {glm::dvec3(1, 2, 3),
          glm::dvec3(2, 3, 4),
          glm::dvec3(3, 4, 5),
          glm::dvec3(1, 2, 3)}},
        {{glm::dvec3(1, 2, 3),
          glm::dvec3(4, 5, 6),
          glm::dvec3(5, 6, 7),
          glm::dvec3(1, 2, 3)}},
        {{glm::dvec3(1, 2, 3),
          glm::dvec3(4, 5, 6),
          glm::dvec3(7, 8, 9),
          glm::dvec3(1, 2, 3)}}};

    CHECK(polygons == polygonsExpected);
  }
}

TEST_CASE("GeoJsonObject:allOfType") {
  SUBCASE("Feature") {
    GeoJsonObject featuresObj = parseGeoJsonObject(R"==(
    {
      "type": "FeatureCollection",
      "features": [
        { "type": "Feature", "properties": null, "id": 0 },
        { "type": "Feature", "properties": null, "id": 1 },
        { "type": "Feature", "properties": null, "id": 2 },
        { "type": "Feature", "properties": null, "id": 3 },
        { "type": "Feature", "properties": null, "id": 4 }
      ]
    }
    )==");

    const std::vector<int64_t> expectedIds = {0, 1, 2, 3, 4};
    ConstGeoJsonObjectTypeIterator<GeoJsonFeature> featureIt =
        featuresObj.allOfType<GeoJsonFeature>().begin();
    for (size_t i = 0; i < expectedIds.size(); i++) {
      const int64_t* pId = std::get_if<int64_t>(&featureIt->id);
      REQUIRE(pId);
      CHECK(*pId == expectedIds[i]);
      ++featureIt;
    }
  }

  SUBCASE("Point") {
    GeoJsonObject pointsObj = parseGeoJsonObject(R"==(
    {
      "type": "GeometryCollection",
      "geometries": [
        { "type": "Point", "coordinates": [ 1, 2, 3 ] },
        { "type": "Point", "coordinates": [ 2, 3, 4 ] },
        { "type": "Point", "coordinates": [ 3, 4, 5 ] },
        { "type": "Point", "coordinates": [ 4, 5, 6 ] },
        { "type": "Point", "coordinates": [ 5, 6, 7 ] }
      ]
    }
    )==");

    const std::vector<glm::dvec3> expectedCoordinates{
        glm::dvec3(1, 2, 3),
        glm::dvec3(2, 3, 4),
        glm::dvec3(3, 4, 5),
        glm::dvec3(4, 5, 6),
        glm::dvec3(5, 6, 7)};
    ConstGeoJsonObjectTypeIterator<GeoJsonPoint> pointsIt =
        pointsObj.allOfType<GeoJsonPoint>().begin();
    for (size_t i = 0; i < expectedCoordinates.size(); i++) {
      CHECK(pointsIt->coordinates == expectedCoordinates[i]);
      ++pointsIt;
    }
  }
}

TEST_CASE("GeoJsonObject:visit") {
  GeoJsonObject pointsObj = parseGeoJsonObject(R"==(
    {
      "type": "Point",
      "coordinates": [1, 2, 3] 
    })==");

  SUBCASE("visitor returns void") {
    bool visited = false;
    pointsObj.visit([&visited](const auto&) { visited = true; });
    CHECK(visited);
  }

  SUBCASE("visitor returns value") {
    CHECK(pointsObj.visit([](const auto&) {
      return std::string("test");
    }) == "test");
  }

  SUBCASE("visitor returns reference") {
    std::string s = "test";
    const std::string& result =
        pointsObj.visit([&s](const auto&) -> const std::string& { return s; });
    CHECK(&s == &result);
  }
}
