#include <CesiumVectorData/GeoJsonObject.h>

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
GeoJsonFeature::GeoJsonFeature(GeoJsonFeature&& rhs) noexcept
    : id(std::move(rhs.id)),
      geometry(std::move(rhs.geometry)),
      properties(std::move(rhs.properties)),
      boundingBox(std::move(rhs.boundingBox)),
      foreignMembers(std::move(rhs.foreignMembers)),
      style(std::move(rhs.style)) {}

namespace {
struct CopyConstructorVisitor {
  GeoJsonObjectVariant operator()(const GeoJsonPoint& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(const GeoJsonMultiPoint& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(const GeoJsonLineString& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(const GeoJsonMultiLineString& rhs) {
    return rhs;
  }
  GeoJsonObjectVariant operator()(const GeoJsonPolygon& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(const GeoJsonMultiPolygon& rhs) {
    return rhs;
  }
  GeoJsonObjectVariant operator()(const GeoJsonGeometryCollection& rhs) {
    return rhs;
  }
  GeoJsonObjectVariant operator()(const GeoJsonFeature& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(const GeoJsonFeatureCollection& rhs) {
    return rhs;
  }
};
struct MoveConstructorVisitor {
  GeoJsonObjectVariant operator()(GeoJsonPoint&& rhs) { return std::move(rhs); }
  GeoJsonObjectVariant operator()(GeoJsonMultiPoint&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonLineString&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonMultiLineString&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonPolygon&& rhs) { return rhs; }
  GeoJsonObjectVariant operator()(GeoJsonMultiPolygon&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonGeometryCollection&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonFeature&& rhs) {
    return std::move(rhs);
  }
  GeoJsonObjectVariant operator()(GeoJsonFeatureCollection&& rhs) {
    return std::move(rhs);
  }
};
} // namespace

GeoJsonFeature::GeoJsonFeature(const GeoJsonFeature& rhs)
    : id(rhs.id),
      geometry(std::make_unique<GeoJsonObject>(*rhs.geometry)),
      properties(rhs.properties),
      boundingBox(rhs.boundingBox),
      foreignMembers(rhs.foreignMembers),
      style(rhs.style) {}

GeoJsonFeature::GeoJsonFeature(
    std::variant<std::monostate, std::string, int64_t> id_,
    std::unique_ptr<GeoJsonObject>&& geometry_,
    std::optional<CesiumUtility::JsonValue::Object>&& properties_,
    std::optional<CesiumGeospatial::BoundingRegion>&& boundingBox_,
    CesiumUtility::JsonValue::Object&& foreignMembers_,
    std::optional<CesiumVectorData::VectorStyle>&& style_)
    : id(std::move(id_)),
      geometry(std::move(geometry_)),
      properties(std::move(properties_)),
      boundingBox(std::move(boundingBox_)),
      foreignMembers(std::move(foreignMembers_)),
      style(std::move(style_)) {}

GeoJsonFeature& GeoJsonFeature::operator=(const GeoJsonFeature& rhs) {
  this->id = rhs.id;
  this->geometry = std::make_unique<GeoJsonObject>(*rhs.geometry);
  this->properties = rhs.properties;
  this->boundingBox = rhs.boundingBox;
  this->foreignMembers = rhs.foreignMembers;
  this->style = rhs.style;
  return *this;
}

GeoJsonFeature& GeoJsonFeature::operator=(GeoJsonFeature&& rhs) noexcept {
  std::swap(this->id, rhs.id);
  std::swap(this->geometry, rhs.geometry);
  std::swap(this->properties, rhs.properties);
  std::swap(this->boundingBox, rhs.boundingBox);
  std::swap(this->foreignMembers, rhs.foreignMembers);
  std::swap(this->style, rhs.style);
  return *this;
}
} // namespace CesiumVectorData