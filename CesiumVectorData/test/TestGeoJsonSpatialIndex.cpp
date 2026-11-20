#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/GeoJsonSpatialIndex.h>
#include <CesiumVectorData/VectorStyle.h>

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace {
// The length of one degree along the WGS84 equator, in meters.
const double METERS_PER_DEGREE = 6378137.0 * Math::OnePi / 180.0;

std::shared_ptr<GeoJsonDocument> parse(const std::string& json) {
  Result<GeoJsonDocument> result =
      GeoJsonDocument::fromGeoJson(std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(json.data()),
          json.size()));
  REQUIRE(!result.errors.hasErrors());
  REQUIRE(result.value);
  return std::make_shared<GeoJsonDocument>(std::move(*result.value));
}

Cartographic degrees(double longitude, double latitude) {
  return Cartographic::fromDegrees(longitude, latitude);
}

GeoJsonPickOptions tolerance(double meters) {
  GeoJsonPickOptions options;
  options.toleranceMeters = meters;
  return options;
}

VectorStyle lineStyle(double width, LineWidthMode mode) {
  VectorStyle style;
  style.line.width = width;
  style.line.widthMode = mode;
  return style;
}

std::vector<GeoJsonObject>& features(GeoJsonDocument& document) {
  return document.rootObject.get<GeoJsonFeatureCollection>().features;
}

int64_t featureId(const GeoJsonPickResult& result) {
  REQUIRE(result.pFeature);
  return std::get<int64_t>(result.pFeature->get<GeoJsonFeature>().id);
}

std::vector<int64_t> featureIds(const std::vector<GeoJsonPickResult>& results) {
  std::vector<int64_t> ids;
  for (const GeoJsonPickResult& result : results) {
    ids.emplace_back(featureId(result));
  }
  return ids;
}
} // namespace

TEST_CASE("GeoJsonSpatialIndex picks points") {
  const std::shared_ptr<GeoJsonDocument> pDocument =
      parse(R"({"type": "Point", "coordinates": [0, 0]})");

  VectorStyle style;
  style.point.radius = 5.0;
  const GeoJsonSpatialIndex index(pDocument, style);
  CHECK(index.size() == 1);

  SUBCASE("at the point") {
    const std::vector<GeoJsonPickResult> results = index.pick(degrees(0, 0));
    REQUIRE(results.size() == 1);
    CHECK(results[0].pGeometry == &pDocument->rootObject);
    CHECK(results[0].pFeature == nullptr);
    CHECK(results[0].distanceMeters == doctest::Approx(0.0));
  }

  SUBCASE("within the tolerance") {
    const double distance = 0.001 * METERS_PER_DEGREE;
    CHECK(index.pick(degrees(0.001, 0), tolerance(distance - 1.0)).empty());

    const std::vector<GeoJsonPickResult> results =
        index.pick(degrees(0.001, 0), tolerance(distance + 1.0));
    REQUIRE(results.size() == 1);
    CHECK(results[0].distanceMeters == doctest::Approx(distance));
  }

  SUBCASE("within the radius in pixels") {
    // About 111 meters away. A 5 pixel radius at 30 meters per pixel reaches
    // 150 meters.
    GeoJsonPickOptions options;
    CHECK(index.pick(degrees(0.001, 0), options).empty());
    options.metersPerPixel = 30.0;
    CHECK(index.pick(degrees(0.001, 0), options).size() == 1);
    options.metersPerPixel = 20.0;
    CHECK(index.pick(degrees(0.001, 0), options).empty());
  }

  SUBCASE("outline widens the point") {
    VectorStyle outlined = style;
    outlined.point.outline = LineStyle();
    outlined.point.outline->width = 2.0;
    const GeoJsonSpatialIndex outlinedIndex(pDocument, outlined);

    // Reaches 5 + 2 / 2 = 6 pixels, or 120 meters at 20 meters per pixel.
    GeoJsonPickOptions options;
    options.metersPerPixel = 20.0;
    CHECK(outlinedIndex.pick(degrees(0.001, 0), options).size() == 1);
  }
}

TEST_CASE("GeoJsonSpatialIndex picks lines") {
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "LineString", "coordinates": [[0, 0], [1, 0]]})");
  const double distance = 0.0005 * METERS_PER_DEGREE;
  const Cartographic nearLine = degrees(0.5, 0.0005);

  SUBCASE("measures the distance to the nearest segment") {
    const GeoJsonSpatialIndex index(pDocument);
    CHECK(index.pick(nearLine, tolerance(distance - 1.0)).empty());

    const std::vector<GeoJsonPickResult> results =
        index.pick(nearLine, tolerance(distance + 1.0));
    REQUIRE(results.size() == 1);
    CHECK(results[0].distanceMeters == doctest::Approx(distance));
  }

  SUBCASE("measures past the end of the line to its end point") {
    const GeoJsonSpatialIndex index(pDocument);
    const std::vector<GeoJsonPickResult> results =
        index.pick(degrees(1.001, 0), tolerance(1000.0));
    REQUIRE(results.size() == 1);
    CHECK(
        results[0].distanceMeters ==
        doctest::Approx(0.001 * METERS_PER_DEGREE));
  }

  SUBCASE("includes a width in meters") {
    const GeoJsonSpatialIndex wide(
        pDocument,
        lineStyle(2.0 * distance + 2.0, LineWidthMode::Meters));
    CHECK(wide.pick(nearLine).size() == 1);

    const GeoJsonSpatialIndex narrow(
        pDocument,
        lineStyle(2.0 * distance - 2.0, LineWidthMode::Meters));
    CHECK(narrow.pick(nearLine).empty());
  }

  SUBCASE("includes a width in pixels only with metersPerPixel") {
    // A 4 pixel wide line at 30 meters per pixel reaches 60 meters on each
    // side, past the 55.7 meters to the picked position.
    const GeoJsonSpatialIndex index(
        pDocument,
        lineStyle(4.0, LineWidthMode::Pixels));
    GeoJsonPickOptions options;
    CHECK(index.pick(nearLine, options).empty());
    options.metersPerPixel = 30.0;
    CHECK(index.pick(nearLine, options).size() == 1);
    options.metersPerPixel = 25.0;
    CHECK(index.pick(nearLine, options).empty());
  }

  SUBCASE("measures distance in ground meters away from the equator") {
    // At 60 degrees latitude, a degree of longitude is half as long.
    const std::shared_ptr<GeoJsonDocument> pNorthern = parse(R"(
      {"type": "LineString", "coordinates": [[10, 59], [10, 61]]})");
    const GeoJsonSpatialIndex index(pNorthern);
    const std::vector<GeoJsonPickResult> results =
        index.pick(degrees(10.001, 60), tolerance(1000.0));
    REQUIRE(results.size() == 1);
    CHECK(
        results[0].distanceMeters ==
        doctest::Approx(0.0005 * METERS_PER_DEGREE).epsilon(1e-6));
  }

  SUBCASE("picks any line of a MultiLineString") {
    const std::shared_ptr<GeoJsonDocument> pMulti = parse(R"(
      {"type": "MultiLineString",
       "coordinates": [[[0, 0], [1, 0]], [[0, 5], [1, 5]]]})");
    const GeoJsonSpatialIndex index(pMulti);
    CHECK(index.pick(degrees(0.5, 5), tolerance(1.0)).size() == 1);
    CHECK(index.pick(degrees(0.5, 2.5), tolerance(1.0)).empty());
  }
}

TEST_CASE("GeoJsonSpatialIndex picks polygons") {
  // A square with a square hole in the middle.
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "Polygon", "coordinates": [
      [[0, 0], [10, 0], [10, 10], [0, 10], [0, 0]],
      [[4, 4], [4, 6], [6, 6], [6, 4], [4, 4]]
    ]})");
  const GeoJsonSpatialIndex index(pDocument);

  SUBCASE("inside the polygon") {
    const std::vector<GeoJsonPickResult> results = index.pick(degrees(2, 2));
    REQUIRE(results.size() == 1);
    CHECK(results[0].distanceMeters == 0.0);
  }

  SUBCASE("not inside a hole") {
    CHECK(index.pick(degrees(5, 5)).empty());

    // The nearest edges of the hole are a degree of longitude away, so a
    // large enough tolerance reaches them.
    const double edgeDistance =
        METERS_PER_DEGREE * std::cos(Math::degreesToRadians(5.0));
    const std::vector<GeoJsonPickResult> results =
        index.pick(degrees(5, 5), tolerance(edgeDistance + 1.0));
    REQUIRE(results.size() == 1);
    CHECK(results[0].distanceMeters == doctest::Approx(edgeDistance));
    CHECK(index.pick(degrees(5, 5), tolerance(edgeDistance - 1.0)).empty());
  }

  SUBCASE("outside the polygon") {
    CHECK(index.pick(degrees(11, 5)).empty());
    CHECK(index.pick(degrees(10.001, 5), tolerance(120.0)).size() == 1);
  }

  SUBCASE("outside the polygon but within its outline width") {
    VectorStyle style;
    style.polygon.outline = LineStyle();
    style.polygon.outline->width = 300.0;
    style.polygon.outline->widthMode = LineWidthMode::Meters;
    const GeoJsonSpatialIndex outlined(pDocument, style);
    CHECK(outlined.pick(degrees(10.001, 5)).size() == 1);
    CHECK(outlined.pick(degrees(10.002, 5)).empty());
  }

  SUBCASE("any polygon of a MultiPolygon") {
    const std::shared_ptr<GeoJsonDocument> pMulti = parse(R"(
      {"type": "MultiPolygon", "coordinates": [
        [[[0, 0], [1, 0], [1, 1], [0, 1], [0, 0]]],
        [[[5, 5], [6, 5], [6, 6], [5, 6], [5, 5]]]
      ]})");
    const GeoJsonSpatialIndex multiIndex(pMulti);
    CHECK(multiIndex.pick(degrees(0.5, 0.5)).size() == 1);
    CHECK(multiIndex.pick(degrees(5.5, 5.5)).size() == 1);
    CHECK(multiIndex.pick(degrees(3, 3)).empty());
  }

  SUBCASE("rings that are not closed") {
    const std::shared_ptr<GeoJsonDocument> pOpen = parse(R"(
      {"type": "Polygon", "coordinates": [[[0, 0], [10, 0], [10, 10], [0, 10]]]})");
    const GeoJsonSpatialIndex openIndex(pOpen);
    CHECK(openIndex.pick(degrees(5, 5)).size() == 1);
    // Near the missing closing edge, from (0, 10) to (0, 0).
    CHECK(openIndex.pick(degrees(-0.001, 5), tolerance(120.0)).size() == 1);
  }
}

TEST_CASE("GeoJsonSpatialIndex returns features") {
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "FeatureCollection", "features": [
      {"type": "Feature", "id": 1, "properties": {"name": "a"},
       "geometry": {"type": "Point", "coordinates": [0, 0]}},
      {"type": "Feature", "id": 2, "properties": null,
       "geometry": {"type": "GeometryCollection", "geometries": [
         {"type": "Point", "coordinates": [1, 0]},
         {"type": "LineString", "coordinates": [[2, 0], [3, 0]]}
       ]}},
      {"type": "Feature", "id": 3, "properties": null, "geometry": null}
    ]})");
  const GeoJsonSpatialIndex index(pDocument);
  CHECK(index.size() == 3);

  std::vector<GeoJsonPickResult> results = index.pick(degrees(0, 0));
  REQUIRE(results.size() == 1);
  CHECK(featureId(results[0]) == 1);
  CHECK(results[0].pFeature == &features(*pDocument)[0]);
  CHECK(
      results[0].pGeometry ==
      features(*pDocument)[0].get<GeoJsonFeature>().geometry.get());

  // Geometry in a GeometryCollection belongs to the Feature that contains the
  // collection.
  results = index.pick(degrees(1, 0));
  REQUIRE(results.size() == 1);
  CHECK(featureId(results[0]) == 2);
  CHECK(results[0].pGeometry->isType<GeoJsonPoint>());

  results = index.pick(degrees(2.5, 0));
  REQUIRE(results.size() == 1);
  CHECK(featureId(results[0]) == 2);
  CHECK(results[0].pGeometry->isType<GeoJsonLineString>());
}

TEST_CASE("GeoJsonSpatialIndex resolves styles like the overlay") {
  // Four lines, each 0.001 degrees (about 111 meters) south of the position
  // picked above it.
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "FeatureCollection", "features": [
      {"type": "Feature", "id": 1, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[0, 0], [1, 0]]}},
      {"type": "Feature", "id": 2, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[2, 0], [3, 0]]}},
      {"type": "Feature", "id": 3, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[4, 0], [5, 0]]}},
      {"type": "Feature", "id": 4, "properties": null,
       "geometry": {"type": "GeometryCollection", "geometries": [
         {"type": "LineString", "coordinates": [[6, 0], [7, 0]]}
       ]}}
    ]})");

  const VectorStyle wide = lineStyle(300.0, LineWidthMode::Meters);
  const VectorStyle narrow = lineStyle(1.0, LineWidthMode::Meters);

  // The collection is wide. Feature 2 is narrow, and feature 3 is wide but
  // its geometry is narrow. The GeometryCollection in feature 4 is narrow.
  pDocument->rootObject.getStyle() = wide;
  features(*pDocument)[1].getStyle() = narrow;
  features(*pDocument)[2].getStyle() = wide;
  features(*pDocument)[2].get<GeoJsonFeature>().geometry->getStyle() = narrow;
  features(*pDocument)[3].get<GeoJsonFeature>().geometry->getStyle() = narrow;

  const GeoJsonSpatialIndex index(pDocument);
  CHECK(featureIds(index.pick(degrees(0.5, 0.001))) == std::vector<int64_t>{1});
  CHECK(index.pick(degrees(2.5, 0.001)).empty());
  CHECK(index.pick(degrees(4.5, 0.001)).empty());
  CHECK(index.pick(degrees(6.5, 0.001)).empty());

  SUBCASE("falls back to the default style") {
    pDocument->rootObject.getStyle().reset();
    features(*pDocument)[1].getStyle().reset();
    const GeoJsonSpatialIndex defaulted(pDocument, wide);
    CHECK(defaulted.pick(degrees(0.5, 0.001)).size() == 1);
    CHECK(defaulted.pick(degrees(2.5, 0.001)).size() == 1);
    CHECK(defaulted.pick(degrees(4.5, 0.001)).empty());
  }

  SUBCASE("copies styles when it is built") {
    pDocument->rootObject.getStyle() = narrow;
    CHECK(index.pick(degrees(0.5, 0.001)).size() == 1);
  }
}

TEST_CASE("GeoJsonSpatialIndex orders results") {
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "FeatureCollection", "features": [
      {"type": "Feature", "id": 1, "properties": null,
       "geometry": {"type": "Polygon", "coordinates":
         [[[-1, -1], [1, -1], [1, 1], [-1, 1], [-1, -1]]]}},
      {"type": "Feature", "id": 2, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[-1, 0.0005], [1, 0.0005]]}},
      {"type": "Feature", "id": 3, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[-1, 0.0002], [1, 0.0002]]}},
      {"type": "Feature", "id": 4, "properties": null,
       "geometry": {"type": "Point", "coordinates": [0, 0.0008]}},
      {"type": "Feature", "id": 5, "properties": null,
       "geometry": {"type": "Polygon", "coordinates":
         [[[-2, -2], [2, -2], [2, 2], [-2, 2], [-2, -2]]]}}
    ]})");
  const GeoJsonSpatialIndex index(pDocument);

  // Points, then lines nearest first, then polygons in document order.
  GeoJsonPickOptions options = tolerance(100.0);
  CHECK(
      featureIds(index.pick(degrees(0, 0), options)) ==
      std::vector<int64_t>{4, 3, 2, 1, 5});

  options.maxResults = 2;
  CHECK(
      featureIds(index.pick(degrees(0, 0), options)) ==
      std::vector<int64_t>{4, 3});

  options.maxResults = 0;
  CHECK(index.pick(degrees(0, 0), options).empty());
}

TEST_CASE("GeoJsonSpatialIndex picks across the antimeridian") {
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "FeatureCollection", "features": [
      {"type": "Feature", "id": 1, "properties": null,
       "geometry": {"type": "Point", "coordinates": [179.9995, 0]}},
      {"type": "Feature", "id": 2, "properties": null,
       "geometry": {"type": "Point", "coordinates": [-179.9995, 10]}}
    ]})");
  const GeoJsonSpatialIndex index(pDocument);

  // Each point is about 111 meters from the position, across the
  // antimeridian.
  std::vector<GeoJsonPickResult> results =
      index.pick(degrees(-179.9995, 0), tolerance(120.0));
  REQUIRE(results.size() == 1);
  CHECK(featureId(results[0]) == 1);
  CHECK(
      results[0].distanceMeters ==
      doctest::Approx(0.001 * METERS_PER_DEGREE).epsilon(1e-6));

  results = index.pick(degrees(179.9995, 10), tolerance(120.0));
  REQUIRE(results.size() == 1);
  CHECK(featureId(results[0]) == 2);
}

TEST_CASE("GeoJsonSpatialIndex picks within a rectangle") {
  const std::shared_ptr<GeoJsonDocument> pDocument = parse(R"(
    {"type": "FeatureCollection", "features": [
      {"type": "Feature", "id": 1, "properties": null,
       "geometry": {"type": "Point", "coordinates": [1, 1]}},
      {"type": "Feature", "id": 2, "properties": null,
       "geometry": {"type": "Point", "coordinates": [5, 5]}},
      {"type": "Feature", "id": 3, "properties": null,
       "geometry": {"type": "LineString", "coordinates": [[-1, 2], [3, 2]]}},
      {"type": "Feature", "id": 4, "properties": null,
       "geometry": {"type": "Polygon", "coordinates": [
         [[10, 10], [20, 10], [20, 20], [10, 20], [10, 10]],
         [[14, 14], [14, 16], [16, 16], [16, 14], [14, 14]]
       ]}},
      {"type": "Feature", "id": 5, "properties": null,
       "geometry": {"type": "MultiPoint", "coordinates": [[179.5, 0], [30, 30]]}},
      {"type": "Feature", "id": 6, "properties": null,
       "geometry": {"type": "Point", "coordinates": [-179.5, 0]}}
    ]})");
  const GeoJsonSpatialIndex index(pDocument);

  const auto pickIds =
      [&](double west, double south, double east, double north) {
        return featureIds(
            index.pick(GlobeRectangle::fromDegrees(west, south, east, north)));
      };

  CHECK(pickIds(0, 0, 2, 3) == std::vector<int64_t>{1, 3});
  // The line crosses the rectangle without any vertex inside it.
  CHECK(pickIds(0, 1.5, 0.5, 2.5) == std::vector<int64_t>{3});
  CHECK(pickIds(0, 2.5, 4, 4).empty());

  // Inside the polygon, inside its hole, and overlapping only its edge.
  CHECK(pickIds(11, 11, 12, 12) == std::vector<int64_t>{4});
  CHECK(pickIds(14.5, 14.5, 15.5, 15.5).empty());
  CHECK(pickIds(19, 5, 25, 11) == std::vector<int64_t>{4});
  // Containing the whole polygon.
  CHECK(pickIds(9, 9, 21, 21) == std::vector<int64_t>{4});

  // Crossing the antimeridian.
  CHECK(pickIds(179, -1, -179, 1) == std::vector<int64_t>{5, 6});

  CHECK(pickIds(-180, -90, 180, 90) == std::vector<int64_t>{1, 2, 3, 4, 5, 6});
  CHECK(
      featureIds(
          index.pick(GlobeRectangle::fromDegrees(-180, -90, 180, 90), 2)) ==
      std::vector<int64_t>{1, 2});
}

TEST_CASE("GeoJsonSpatialIndex handles many geometries") {
  // A grid of points large enough to build several levels of the tree.
  const int32_t size = 60;
  std::string features;
  for (int32_t y = 0; y < size; ++y) {
    for (int32_t x = 0; x < size; ++x) {
      if (!features.empty()) {
        features += ",";
      }
      features += R"({"type": "Feature", "properties": null, "id": )" +
                  std::to_string(y * size + x) +
                  R"(, "geometry": {"type": "Point", "coordinates": [)" +
                  std::to_string(x * 0.1 - 3.0) + "," +
                  std::to_string(y * 0.1 - 3.0) + "]}}";
    }
  }
  const std::shared_ptr<GeoJsonDocument> pDocument =
      parse(R"({"type": "FeatureCollection", "features": [)" + features + "]}");
  const GeoJsonSpatialIndex index(pDocument);
  REQUIRE(index.size() == size_t(size * size));

  for (int32_t y = 0; y < size; y += 7) {
    for (int32_t x = 0; x < size; x += 5) {
      const std::vector<GeoJsonPickResult> results = index.pick(
          degrees(x * 0.1 - 3.0 + 0.0001, y * 0.1 - 3.0),
          tolerance(100.0));
      REQUIRE(results.size() == 1);
      CHECK(featureId(results[0]) == y * size + x);
    }
  }

  // Columns 10 to 19 and rows 20 to 24.
  const std::vector<int64_t> ids = featureIds(
      index.pick(GlobeRectangle::fromDegrees(-2.05, -1.05, -1.05, -0.55)));
  std::vector<int64_t> expected;
  for (int32_t y = 20; y < 25; ++y) {
    for (int32_t x = 10; x < 20; ++x) {
      expected.emplace_back(y * size + x);
    }
  }
  CHECK(ids == expected);
}

TEST_CASE("GeoJsonSpatialIndex handles empty documents") {
  const GeoJsonSpatialIndex noDocument(nullptr);
  CHECK(noDocument.size() == 0);
  CHECK(noDocument.pick(degrees(0, 0), tolerance(1e6)).empty());
  CHECK(noDocument.pick(GlobeRectangle::MAXIMUM).empty());

  const GeoJsonSpatialIndex emptyCollection(
      parse(R"({"type": "FeatureCollection", "features": []})"));
  CHECK(emptyCollection.size() == 0);
  CHECK(emptyCollection.pick(degrees(0, 0), tolerance(1e6)).empty());

  const GeoJsonSpatialIndex emptyPoints(
      parse(R"({"type": "MultiPoint", "coordinates": []})"));
  CHECK(emptyPoints.size() == 0);
  CHECK(emptyPoints.pick(GlobeRectangle::MAXIMUM).empty());
}
