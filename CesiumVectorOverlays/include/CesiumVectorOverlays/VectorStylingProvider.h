#pragma once

#include "Library.h"

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltf/Model.h>
#include <CesiumVectorData/VectorStyle.h>

namespace CesiumVectorOverlays {

class CESIUMVECTOROVERLAYS_API VectorStylingProvider {
public:
  virtual void onStylingBegin(const CesiumGltf::Model& model) = 0;
  virtual std::optional<CesiumVectorData::VectorStyle> onStylePoint(
      int64_t featureId,
      const CesiumGeospatial::Cartographic& point) = 0;
  virtual std::optional<CesiumVectorData::VectorStyle> onStylePolyline(
      int64_t featureId,
      const std::vector<CesiumGeospatial::Cartographic>& polyline) = 0;
  virtual std::optional<CesiumVectorData::VectorStyle> onStylePolygon(
      int64_t featureId,
      const std::vector<CesiumGeospatial::Cartographic>& polygon) = 0;
};

} // namespace CesiumVectorOverlays