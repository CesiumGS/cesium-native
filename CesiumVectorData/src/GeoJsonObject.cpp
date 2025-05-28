#include <CesiumVectorData/GeoJsonObject.h>

namespace CesiumVectorData {
GeoJsonObjectType GeoJsonObject::getType() const {
  return std::visit([](const auto& value) { return value.TYPE; }, this->value);
}
GeoJsonObjectIterator GeoJsonObject::begin() {
  return GeoJsonObjectIterator(*this);
}
GeoJsonObjectIterator GeoJsonObject::end() { return GeoJsonObjectIterator(); }
ConstGeoJsonObjectIterator GeoJsonObject::begin() const {
  return ConstGeoJsonObjectIterator(*this);
}
ConstGeoJsonObjectIterator GeoJsonObject::end() const {
  return ConstGeoJsonObjectIterator();
}
} // namespace CesiumVectorData