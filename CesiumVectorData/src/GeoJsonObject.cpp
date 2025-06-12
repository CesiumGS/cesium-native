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

GeoJsonFeature::GeoJsonFeature(GeoJsonFeature&& rhs) noexcept
    : id(std::move(rhs.id)),
      geometry(std::move(rhs.geometry)),
      properties(std::move(rhs.properties)),
      boundingBox(std::move(rhs.boundingBox)),
      foreignMembers(std::move(rhs.foreignMembers)),
      style(std::move(rhs.style)) {}

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