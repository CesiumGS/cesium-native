#pragma once

#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumUtility/JsonValue.h"

#include <cstdint>
#include <span>

namespace CesiumVectorData {
class GeoJsonDocument;

enum class GeoJsonObjectType : uint8_t {
  Point = 0,
  MultiPoint = 1,
  LineString = 2,
  MultiLineString = 3,
  Polygon = 4,
  MultiPolygon = 5,
  GeometryCollection = 6,
  Feature = 7,
  FeatureCollection = 8
};

struct GeoJsonObjectDescriptor {
  GeoJsonObjectType type = GeoJsonObjectType::Point;
  int32_t dataStartIndex = -1;
  int32_t dataEndIndex = -1;
  int32_t boundingBoxIndex = -1;
  int32_t foreignMembersIndex = -1;

  std::span<CesiumGeospatial::Cartographic> getPoints(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  std::span<struct GeoJsonLineString> getLineStrings(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  std::span<struct GeoJsonPolygon> getPolygons(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  std::span<GeoJsonObjectDescriptor> getGeometries(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  std::span<class GeoJsonFeature> getFeatures(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  bool hasBoundingBox(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  std::optional<CesiumGeospatial::BoundingRegion> getBoundingBox(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  bool hasForeignMembers(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

  CesiumUtility::JsonValue::Object getForeignMembers(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;

private:
  template <typename T> bool areIndicesValid(const std::vector<T>& vec) const {
    return this->dataStartIndex >= 0 &&
           this->dataStartIndex < static_cast<int32_t>(vec.size()) &&
           this->dataEndIndex >= 0 &&
           this->dataEndIndex < static_cast<int32_t>(vec.size());
  }
};
} // namespace CesiumVectorData