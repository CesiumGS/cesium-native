#pragma once

#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltf/Model.h>
#include <CesiumVectorData/VectorStyle.h>

namespace CesiumVectorOverlays {

class CESIUMVECTOROVERLAYS_API VectorStylingProvider {
public:
  virtual CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>> onStylePoints(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<CesiumGeospatial::Cartographic>& points) = 0;
  virtual CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>> onStylePolylines(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polylines) = 0;
  virtual CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>> onStylePolygons(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polygons) = 0;
};

} // namespace CesiumVectorOverlays