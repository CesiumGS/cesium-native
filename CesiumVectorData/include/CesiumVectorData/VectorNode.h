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
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct MultiPoint {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::MultiPoint;

  std::vector<CesiumGeospatial::Cartographic> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct LineString {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::LineString;

  std::vector<CesiumGeospatial::Cartographic> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct MultiLineString {
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::MultiLineString;

  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct Polygon {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct MultiPolygon {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  std::vector<std::vector<std::vector<CesiumGeospatial::Cartographic>>>
      coordinates;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
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
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct Feature {
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Feature;

  std::variant<std::string, int64_t, std::monostate> id;
  std::optional<GeometryPrimitive> geometry;
  std::optional<CesiumUtility::JsonValue::Object> properties;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
};

struct FeatureCollection {
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::FeatureCollection;

  std::vector<Feature> features;
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox;
  CesiumUtility::JsonValue::Object foreignMembers;
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
} // namespace CesiumVectorData