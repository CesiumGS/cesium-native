#pragma once

#include "Library.h"

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltf/Model.h>
#include <CesiumVectorData/VectorStyle.h>

namespace CesiumVectorOverlays {

class CESIUMVECTOROVERLAYS_API VectorStylingProvider {
public:
  virtual bool onStylePoints(
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<CesiumGeospatial::Cartographic>& points,
      std::vector<std::optional<CesiumVectorData::VectorStyle>>& outStyles) = 0;
  virtual bool onStylePolylines(
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polylines,
      std::vector<std::optional<CesiumVectorData::VectorStyle>>& outStyles) = 0;
  virtual bool onStylePolygons(
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polygons,
      std::vector<std::optional<CesiumVectorData::VectorStyle>>& outStyles) = 0;
};

} // namespace CesiumVectorOverlays