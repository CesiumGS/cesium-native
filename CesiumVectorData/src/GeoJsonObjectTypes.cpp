#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>

#include <string_view>
#include <variant>

using namespace CesiumUtility;

namespace CesiumVectorData {
std::string_view geoJsonObjectTypeToString(GeoJsonObjectType type) {
  constexpr std::string_view values[] = {
      "Point",
      "MultiPoint",
      "LineString",
      "MultiLineString",
      "Polygon",
      "MultiPolygon",
      "GeometryCollection",
      "Feature",
      "FeatureCollection"};
  if (size_t(type) >= 0 && size_t(type) < sizeof(values)) {
    return values[size_t(type)];
  } else {
    constexpr std::string_view unknown = "Unknown";
    return unknown;
  }
}

GeoJsonFeature::GeoJsonFeature(GeoJsonFeature&& rhs) noexcept
    : id(std::move(rhs.id)),
      geometry(std::move(rhs.geometry)),
      properties(std::move(rhs.properties)),
      boundingBox(std::move(rhs.boundingBox)),
      foreignMembers(std::move(rhs.foreignMembers)) {}

GeoJsonFeature::GeoJsonFeature(const GeoJsonFeature& rhs)
    : id(rhs.id),
      geometry(std::make_unique<GeoJsonObject>(*rhs.geometry)),
      properties(rhs.properties),
      boundingBox(rhs.boundingBox),
      foreignMembers(rhs.foreignMembers) {}

GeoJsonFeature::GeoJsonFeature(
    std::variant<std::monostate, std::string, int64_t> id_,
    std::unique_ptr<GeoJsonObject>&& geometry_,
    std::optional<CesiumUtility::JsonValue::Object>&& properties_,
    std::optional<CesiumGeometry::AxisAlignedBox>&& boundingBox_,
    CesiumUtility::JsonValue::Object&& foreignMembers_)
    : id(std::move(id_)),
      geometry(std::move(geometry_)),
      properties(std::move(properties_)),
      boundingBox(std::move(boundingBox_)),
      foreignMembers(std::move(foreignMembers_)) {}

GeoJsonFeature& GeoJsonFeature::operator=(const GeoJsonFeature& rhs) {
  this->id = rhs.id;
  this->geometry = std::make_unique<GeoJsonObject>(*rhs.geometry);
  this->properties = rhs.properties;
  this->boundingBox = rhs.boundingBox;
  this->foreignMembers = rhs.foreignMembers;
  return *this;
}

GeoJsonFeature& GeoJsonFeature::operator=(GeoJsonFeature&& rhs) noexcept {
  std::swap(this->id, rhs.id);
  std::swap(this->geometry, rhs.geometry);
  std::swap(this->properties, rhs.properties);
  std::swap(this->boundingBox, rhs.boundingBox);
  std::swap(this->foreignMembers, rhs.foreignMembers);
  return *this;
}
} // namespace CesiumVectorData