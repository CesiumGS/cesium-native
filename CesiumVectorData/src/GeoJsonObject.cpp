#include <CesiumVectorData/GeoJsonObject.h>

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

GeoJsonObjectType GeoJsonObject::getType() const {
  return std::visit([](const auto& value) { return value.TYPE; }, this->value);
}
} // namespace CesiumVectorData