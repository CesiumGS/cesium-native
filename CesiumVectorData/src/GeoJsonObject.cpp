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
  struct GeoJsonGeometryObjectToObject {
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

  return std::visit(GeoJsonGeometryObjectToObject{}, geometry);
}

GeoJsonObjectPtr
geoJsonGeometryObjectRefToObjectPtr(GeoJsonGeometryObject& geometry) {
  struct GeoJsonGeometryObjectToObject {
    GeoJsonObjectPtr operator()(GeoJsonPoint& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiPoint& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonLineString& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiLineString& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonPolygon& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiPolygon& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonGeometryCollection& lhs) { return &lhs; }
  };

  return std::visit(GeoJsonGeometryObjectToObject{}, geometry);
}

GeoJsonObjectConstPtr geoJsonGeometryObjectConstRefToObjectConstPtr(
    const GeoJsonGeometryObject& geometry) {
  struct GeoJsonGeometryObjectToObject {
    GeoJsonObjectConstPtr operator()(const GeoJsonPoint& lhs) { return &lhs; }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiPoint& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonLineString& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiLineString& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonPolygon& lhs) { return &lhs; }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiPolygon& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonGeometryCollection& lhs) {
      return &lhs;
    }
  };

  return std::visit(GeoJsonGeometryObjectToObject{}, geometry);
}

GeoJsonObjectPtr geoJsonObjectRefToObjectPtr(GeoJsonObject& object) {
  struct GeoJsonGeometryObjectToObject {
    GeoJsonObjectPtr operator()(GeoJsonPoint& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiPoint& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonLineString& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiLineString& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonPolygon& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonMultiPolygon& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonGeometryCollection& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonFeature& lhs) { return &lhs; }
    GeoJsonObjectPtr operator()(GeoJsonFeatureCollection& lhs) { return &lhs; }
  };

  return std::visit(GeoJsonGeometryObjectToObject{}, object);
}

GeoJsonObjectConstPtr
geoJsonObjectConstRefToObjectConstPtr(const GeoJsonObject& object) {
  struct GeoJsonGeometryObjectToObject {
    GeoJsonObjectConstPtr operator()(const GeoJsonPoint& lhs) { return &lhs; }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiPoint& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonLineString& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiLineString& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonPolygon& lhs) { return &lhs; }
    GeoJsonObjectConstPtr operator()(const GeoJsonMultiPolygon& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonGeometryCollection& lhs) {
      return &lhs;
    }
    GeoJsonObjectConstPtr operator()(const GeoJsonFeature& lhs) { return &lhs; }
    GeoJsonObjectConstPtr operator()(const GeoJsonFeatureCollection& lhs) {
      return &lhs;
    }
  };

  return std::visit(GeoJsonGeometryObjectToObject{}, object);
}
} // namespace CesiumVectorData