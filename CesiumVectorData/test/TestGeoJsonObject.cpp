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