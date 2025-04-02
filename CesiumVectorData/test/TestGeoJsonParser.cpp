#include <CesiumVectorData/VectorDocument.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <span>
#include <string>
#include <variant>

using namespace CesiumVectorData;
using namespace CesiumUtility;

namespace {
static std::span<const std::byte> stringToBytes(const std::string& str) {
  return std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
}
} // namespace

TEST_CASE("Parse Point primitives") {
  SUBCASE("2D points") {
    const std::string POINT_STR = R"==(
    {
        "type": "Point",
        "coordinates": [100.0, 0.0]
    }
    )==";

    Result<VectorDocument> doc =
        VectorDocument::fromGeoJson(stringToBytes(POINT_STR));
    CHECK(!doc.errors.hasErrors());
    REQUIRE(doc.value);
    const VectorNode& root = doc.value->getRootNode();
    const Point* pPoint = std::get_if<Point>(&root);
    REQUIRE(pPoint);
    CHECK(Math::radiansToDegrees(pPoint->coordinates.longitude) == 100.0);
    CHECK(Math::radiansToDegrees(pPoint->coordinates.latitude) == 0);
  }

  SUBCASE("3D points") {
    const std::string POINT_STR = R"==(
    {
        "type": "Point",
        "coordinates": [-100.0, 20.0, 500.0]
    }
    )==";

    Result<VectorDocument> doc =
        VectorDocument::fromGeoJson(stringToBytes(POINT_STR));
    CHECK(!doc.errors.hasErrors());
    REQUIRE(doc.value);
    const VectorNode& root = doc.value->getRootNode();
    const Point* pPoint = std::get_if<Point>(&root);
    REQUIRE(pPoint);
    CHECK(Math::radiansToDegrees(pPoint->coordinates.longitude) == -100.0);
    CHECK(Math::radiansToDegrees(pPoint->coordinates.latitude) == 20);
    CHECK(pPoint->coordinates.height == 500);
  }
}

TEST_CASE("Parse LineString primitives") {
  SUBCASE("2D two point line") {
    const std::string POINT_STR = R"==(
      {
        "type": "LineString",
        "coordinates": [
          [100.0, 0.0],
          [101.0, 1.0]
        ]
      }
    )==";

    Result<VectorDocument> doc =
        VectorDocument::fromGeoJson(stringToBytes(POINT_STR));
    CHECK(!doc.errors.hasErrors());
    REQUIRE(doc.value);
    const VectorNode& root = doc.value->getRootNode();
    const LineString* pLineString = std::get_if<LineString>(&root);
    REQUIRE(pLineString);
    CHECK(pLineString->coordinates.size() == 2);
    CHECK(
        Math::radiansToDegrees(pLineString->coordinates[0].longitude) == 100.0);
    CHECK(Math::radiansToDegrees(pLineString->coordinates[0].latitude) == 0);
    CHECK(
        Math::radiansToDegrees(pLineString->coordinates[1].longitude) == 101.0);
    CHECK(Math::radiansToDegrees(pLineString->coordinates[1].latitude) == 1.0);
  }

  SUBCASE("3D line") {
    const std::string POINT_STR = R"==(
      {
        "type": "LineString",
        "coordinates": [
          [100.0, 0.0, -50.0],
          [101.0, 1.0, 800.0],
          [90.0, 91.0, 109.0]
        ]
      }
    )==";

    Result<VectorDocument> doc =
        VectorDocument::fromGeoJson(stringToBytes(POINT_STR));
    CHECK(!doc.errors.hasErrors());
    REQUIRE(doc.value);
    const VectorNode& root = doc.value->getRootNode();
    const LineString* pLineString = std::get_if<LineString>(&root);
    REQUIRE(pLineString);
    CHECK(pLineString->coordinates.size() == 3);
    CHECK(
        Math::radiansToDegrees(pLineString->coordinates[0].longitude) == 100.0);
    CHECK(Math::radiansToDegrees(pLineString->coordinates[0].latitude) == 0);
    CHECK(pLineString->coordinates[0].height == -50.0);
    CHECK(
        Math::radiansToDegrees(pLineString->coordinates[1].longitude) == 101.0);
    CHECK(Math::radiansToDegrees(pLineString->coordinates[1].latitude) == 1.0);
    CHECK(pLineString->coordinates[1].height == 800.0);
    CHECK(
        Math::radiansToDegrees(pLineString->coordinates[2].longitude) == 90.0);
    CHECK(Math::radiansToDegrees(pLineString->coordinates[2].latitude) == 91.0);
    CHECK(pLineString->coordinates[2].height == 109.0);
  }
}