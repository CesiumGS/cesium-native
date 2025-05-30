#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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
  if (size_t(type) < sizeof(values)) {
    return values[size_t(type)];
  } else {
    constexpr std::string_view unknown = "Unknown";
    return unknown;
  }
}

GeoJsonFeature::GeoJsonFeature(const GeoJsonFeature& rhs)
    : id(rhs.id),
      geometry(
          rhs.geometry == nullptr
              ? nullptr
              : std::make_unique<GeoJsonObject>(*rhs.geometry)),
      properties(rhs.properties),
      boundingBox(rhs.boundingBox),
      foreignMembers(rhs.foreignMembers) {}

GeoJsonFeature::GeoJsonFeature(
    std::variant<std::monostate, std::string, int64_t>&& id_,
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
  this->geometry = rhs.geometry == nullptr
                       ? nullptr
                       : std::make_unique<GeoJsonObject>(*rhs.geometry);
  this->properties = rhs.properties;
  this->boundingBox = rhs.boundingBox;
  this->foreignMembers = rhs.foreignMembers;
  return *this;
}
} // namespace CesiumVectorData