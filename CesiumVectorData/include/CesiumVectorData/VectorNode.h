#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>

#include <glm/vec3.hpp>

#include <variant>
#include <vector>

namespace CesiumVectorData {
enum class VectorPrimitiveType : uint8_t {
  Point = 1,
  MultiPoint = 2,
  LineString = 3,
  MultiLineString = 4,
  Polygon = 5,
  MultiPolygon = 6,
  GeometryCollection = 7,
  Feature = 8,
  FeatureCollection = 9
};

std::string vectorPrimitiveTypeToString(VectorPrimitiveType type);

struct Point {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Point;

  CesiumGeospatial::Cartographic coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct MultiPoint {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::MultiPoint;

  std::vector<CesiumGeospatial::Cartographic> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct LineString {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::LineString;

  std::vector<CesiumGeospatial::Cartographic> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct MultiLineString {
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::MultiLineString;

  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct Polygon {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct MultiPolygon {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  std::vector<std::vector<std::vector<CesiumGeospatial::Cartographic>>>
      coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct GeometryCollection;

using GeometryPrimitive = std::variant<
    Point,
    MultiPoint,
    LineString,
    MultiLineString,
    Polygon,
    MultiPolygon,
    GeometryCollection>;

struct GeometryCollection {
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::GeometryCollection;

  std::vector<GeometryPrimitive> geometries;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct Feature {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Feature;

  std::variant<std::string, int64_t, std::monostate> id = std::monostate();
  std::optional<GeometryPrimitive> geometry;
  std::optional<CesiumUtility::JsonValue::Object> properties;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct FeatureCollection {
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::FeatureCollection;

  std::vector<Feature> features;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

using VectorNode = std::variant<
    Point,
    MultiPoint,
    LineString,
    MultiLineString,
    Polygon,
    MultiPolygon,
    GeometryCollection,
    Feature,
    FeatureCollection>;

VectorNode geometryPrimitiveToVectorNode(const GeometryPrimitive& geometry);

bool operator==(const VectorNode& lhs, const VectorNode& rhs);
bool operator==(const GeometryPrimitive& lhs, const GeometryPrimitive& rhs);
bool operator==(const Point& lhs, const VectorNode& rhs);
bool operator==(const MultiPoint& lhs, const VectorNode& rhs);
bool operator==(const LineString& lhs, const VectorNode& rhs);
bool operator==(const MultiLineString& lhs, const VectorNode& rhs);
bool operator==(const Polygon& lhs, const VectorNode& rhs);
bool operator==(const MultiPolygon& lhs, const VectorNode& rhs);
bool operator==(const GeometryCollection& lhs, const VectorNode& rhs);
bool operator==(const Feature& lhs, const Feature& rhs);
bool operator==(const Feature& lhs, const VectorNode& rhs);
bool operator==(const FeatureCollection& lhs, const VectorNode& rhs);
} // namespace CesiumVectorData