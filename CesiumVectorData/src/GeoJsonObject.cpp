#include <CesiumVectorData/GeoJsonObject.h>

#include <stdexcept>
#include <string>
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

  throw new std::invalid_argument("Unknown VectorPrimitiveType value");
}

GeoJsonObject
geoJsonGeometryObjectToObject(const GeoJsonGeometryObject& geometry) {
  struct GeometryPrimitiveToVectorNodeVisitor {
    GeoJsonObject operator()(const GeoJsonPoint& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonMultiPoint& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonLineString& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonMultiLineString& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonPolygon& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonMultiPolygon& lhs) { return lhs; }
    GeoJsonObject operator()(const GeoJsonGeometryCollection& lhs) {
      return lhs;
    }
  };

  return std::visit(GeometryPrimitiveToVectorNodeVisitor{}, geometry);
}
} // namespace CesiumVectorData