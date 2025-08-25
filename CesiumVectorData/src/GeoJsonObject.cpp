#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorStyle.h>

#include <optional>
#include <variant>

namespace CesiumVectorData {
GeoJsonObjectType GeoJsonObject::getType() const {
  return std::visit([](const auto& v) { return v.TYPE; }, this->value);
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

GeoJsonObject::IteratorProvider<ConstGeoJsonPointIterator>
GeoJsonObject::points() const {
  return IteratorProvider<ConstGeoJsonPointIterator>(this);
}

GeoJsonObject::IteratorProvider<ConstGeoJsonLineStringIterator>
GeoJsonObject::lines() const {
  return IteratorProvider<ConstGeoJsonLineStringIterator>(this);
}

GeoJsonObject::IteratorProvider<ConstGeoJsonPolygonIterator>
GeoJsonObject::polygons() const {
  return IteratorProvider<ConstGeoJsonPolygonIterator>(this);
}

const std::optional<CesiumGeometry::AxisAlignedBox>&
GeoJsonObject::getBoundingBox() const {
  return std::visit(
      [](const auto& v)
          -> const std::optional<CesiumGeometry::AxisAlignedBox>& {
        return v.boundingBox;
      },
      this->value);
}

std::optional<CesiumGeometry::AxisAlignedBox>& GeoJsonObject::getBoundingBox() {
  return std::visit(
      [](auto& v) -> std::optional<CesiumGeometry::AxisAlignedBox>& {
        return v.boundingBox;
      },
      this->value);
}

const std::optional<VectorStyle>& GeoJsonObject::getStyle() const {
  return std::visit(
      [](auto& v) -> const std::optional<VectorStyle>& { return v.style; },
      this->value);
}

std::optional<VectorStyle>& GeoJsonObject::getStyle() {
  return std::visit(
      [](auto& v) -> std::optional<VectorStyle>& { return v.style; },
      this->value);
}

const CesiumUtility::JsonValue::Object&
GeoJsonObject::getForeignMembers() const {
  return std::visit(
      [](const auto& v) -> const CesiumUtility::JsonValue::Object& {
        return v.foreignMembers;
      },
      this->value);
}

CesiumUtility::JsonValue::Object& GeoJsonObject::getForeignMembers() {
  return std::visit(
      [](auto& v) -> CesiumUtility::JsonValue::Object& {
        return v.foreignMembers;
      },
      this->value);
}
} // namespace CesiumVectorData