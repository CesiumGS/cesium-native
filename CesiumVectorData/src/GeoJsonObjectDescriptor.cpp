#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObjectDescriptor.h>

#include <cstdint>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumVectorData {
std::span<CesiumGeospatial::Cartographic> GeoJsonObjectDescriptor::getPoints(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if ((this->type != GeoJsonObjectType::Point &&
       this->type != GeoJsonObjectType::MultiPoint) ||
      !this->areIndicesValid(pDocument->_pointData)) {
    return std::span<CesiumGeospatial::Cartographic>();
  }

  return std::span<CesiumGeospatial::Cartographic>(
      pDocument->_pointData.begin() + this->dataStartIndex,
      pDocument->_pointData.begin() + this->dataEndIndex + 1);
}

std::span<GeoJsonLineString> GeoJsonObjectDescriptor::getLineStrings(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if ((this->type != GeoJsonObjectType::LineString &&
       this->type != GeoJsonObjectType::MultiLineString) ||
      !this->areIndicesValid(pDocument->_lineStringData)) {
    return std::span<GeoJsonLineString>();
  }

  return std::span<GeoJsonLineString>(
      pDocument->_lineStringData.begin() + this->dataStartIndex,
      pDocument->_lineStringData.begin() + this->dataEndIndex + 1);
}

std::span<GeoJsonPolygon> GeoJsonObjectDescriptor::getPolygons(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if ((this->type != GeoJsonObjectType::Polygon &&
       this->type != GeoJsonObjectType::MultiPolygon) ||
      !this->areIndicesValid(pDocument->_polygonData)) {
    return std::span<GeoJsonPolygon>();
  }

  return std::span<GeoJsonPolygon>(
      pDocument->_polygonData.begin() + this->dataStartIndex,
      pDocument->_polygonData.begin() + this->dataEndIndex + 1);
}

std::span<GeoJsonObjectDescriptor> GeoJsonObjectDescriptor::getGeometries(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if (this->type != GeoJsonObjectType::GeometryCollection ||
      !this->areIndicesValid(pDocument->_geometryData)) {
    return std::span<GeoJsonObjectDescriptor>();
  }

  return std::span<GeoJsonObjectDescriptor>(
      pDocument->_geometryData.begin() + this->dataStartIndex,
      pDocument->_geometryData.begin() + this->dataEndIndex + 1);
}

std::span<GeoJsonFeature> GeoJsonObjectDescriptor::getFeatures(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if ((this->type != GeoJsonObjectType::Feature &&
       this->type != GeoJsonObjectType::FeatureCollection) ||
      !this->areIndicesValid(pDocument->_featureData)) {
    return std::span<GeoJsonFeature>();
  }

  return std::span<GeoJsonFeature>(
      pDocument->_featureData.begin() + this->dataStartIndex,
      pDocument->_featureData.begin() + this->dataEndIndex + 1);
}

std::optional<CesiumGeospatial::BoundingRegion>
GeoJsonObjectDescriptor::getBoundingBox(
    const IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if (this->boundingBoxIndex < 0 ||
      this->boundingBoxIndex >=
          static_cast<int32_t>(pDocument->_boundingBoxData.size())) {
    return std::nullopt;
  }

  return pDocument
      ->_boundingBoxData[static_cast<size_t>(this->boundingBoxIndex)];
}

CesiumUtility::JsonValue::Object GeoJsonObjectDescriptor::getForeignMembers(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if (this->foreignMembersIndex < 0 ||
      this->foreignMembersIndex >=
          static_cast<int32_t>(pDocument->_foreignMemberData.size())) {
    return CesiumUtility::JsonValue::Object();
  }

  return pDocument
      ->_foreignMemberData[static_cast<size_t>(this->foreignMembersIndex)];
}

bool GeoJsonObjectDescriptor::hasBoundingBox(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  return this->boundingBoxIndex != -1 && this->boundingBoxIndex >= 0 &&
         this->boundingBoxIndex <
             static_cast<int32_t>(pDocument->_boundingBoxData.size());
}

bool GeoJsonObjectDescriptor::hasForeignMembers(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  return this->foreignMembersIndex != -1 && this->foreignMembersIndex >= 0 &&
         this->foreignMembersIndex <
             static_cast<int32_t>(pDocument->_foreignMemberData.size());
}
} // namespace CesiumVectorData