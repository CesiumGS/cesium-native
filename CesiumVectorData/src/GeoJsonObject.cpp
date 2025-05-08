#include "CesiumGeospatial/Cartographic.h"

#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <span>

using namespace CesiumGeospatial;

namespace CesiumVectorData {

const std::span<Cartographic> GeoJsonLineString::getPoints(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if (this->pointStartIndex < 0 ||
      this->pointStartIndex >=
          static_cast<int32_t>(pDocument->_pointData.size()) ||
      this->pointEndIndex < 0 ||
      this->pointEndIndex >=
          static_cast<int32_t>(pDocument->_pointData.size())) {
    return std::span<Cartographic>();
  }

  return std::span<Cartographic>(
      pDocument->_pointData.begin() + this->pointStartIndex,
      pDocument->_pointData.begin() + this->pointEndIndex + 1);
}

const std::span<GeoJsonLineString> GeoJsonPolygon::getLineStrings(
    const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const {
  if (this->lineStringStartIndex < 0 ||
      this->lineStringStartIndex >=
          static_cast<int32_t>(pDocument->_pointData.size()) ||
      this->lineStringEndIndex < 0 ||
      this->lineStringEndIndex >=
          static_cast<int32_t>(pDocument->_pointData.size())) {
    return std::span<GeoJsonLineString>();
  }

  return std::span<GeoJsonLineString>(
      pDocument->_lineStringData.begin() + this->lineStringStartIndex,
      pDocument->_lineStringData.begin() + this->lineStringEndIndex + 1);
}
} // namespace CesiumVectorData