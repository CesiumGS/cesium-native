#include <CesiumVectorData/GeoJsonObject.h>

#include <string>
#include <utility>
#include <variant>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumVectorData {
std::string geoJsonObjectTypeToString(GeoJsonObjectType type) {
  switch (type) {
  case GeoJsonObjectType::Point:
    return "Point";
  case GeoJsonObjectType::MultiPoint:
    return "MultiPoint";
  case GeoJsonObjectType::LineString:
    return "LineString";
  case GeoJsonObjectType::MultiLineString:
    return "MultiLineString";
  case GeoJsonObjectType::Polygon:
    return "Polygon";
  case GeoJsonObjectType::MultiPolygon:
    return "MultiPolygon";
  case GeoJsonObjectType::GeometryCollection:
    return "GeometryCollection";
  case GeoJsonObjectType::Feature:
    return "Feature";
  case GeoJsonObjectType::FeatureCollection:
    return "FeatureCollection";
  }

  return "Unknown";
}

GeoJsonObjectType GeoJsonObject::getType() const {
  struct GeoJsonObjectTypeVisitor {
    GeoJsonObjectType operator()(const GeoJsonPoint& lhs) { return lhs.TYPE; }
    GeoJsonObjectType operator()(const GeoJsonMultiPoint& lhs) {
      return lhs.TYPE;
    }
    GeoJsonObjectType operator()(const GeoJsonLineString& lhs) {
      return lhs.TYPE;
    }
    GeoJsonObjectType operator()(const GeoJsonMultiLineString& lhs) {
      return lhs.TYPE;
    }
    GeoJsonObjectType operator()(const GeoJsonPolygon& lhs) { return lhs.TYPE; }
    GeoJsonObjectType operator()(const GeoJsonMultiPolygon& lhs) {
      return lhs.TYPE;
    }
    GeoJsonObjectType operator()(const GeoJsonGeometryCollection& lhs) {
      return lhs.TYPE;
    }
    GeoJsonObjectType operator()(const GeoJsonFeature& lhs) { return lhs.TYPE; }
    GeoJsonObjectType operator()(const GeoJsonFeatureCollection& lhs) {
      return lhs.TYPE;
    }
  };

  return std::visit(GeoJsonObjectTypeVisitor{}, this->value);
}
} // namespace CesiumVectorData