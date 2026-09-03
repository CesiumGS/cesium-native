#pragma once

#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltf/Model.h>
#include <CesiumVectorData/VectorStyle.h>

namespace CesiumVectorOverlays {

/**
 * @brief An interface for providing styling information for vector features.
 */
class CESIUMVECTOROVERLAYS_API VectorStylingProvider {
public:
  /**
   * @brief Styles a set of point features.
   *
   * @param asyncSystem The async system.
   * @param model The glTF model containing the features and metadata
   * information.
   * @param featureIds The feature IDs of the points to style.
   * @param points The geometry of the points to style. This will be the same
   * size as `featureIds`, and each point corresponds to the feature ID at the
   * same index.
   * @returns A future that resolves to a vector of optional PointStyle
   * objects, one for each point feature. The vector should be the same size as
   * `featureIds and `points`. If `std::nullopt` is provided for a feature, the
   * feature will use the default style. If the returned vector is empty or does
   * not match the size of `featureIds` and `points`, the default style will be
   * used for all features.
   */
  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::PointStyle>>>
  onStylePoints(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<CesiumGeospatial::Cartographic>& points) = 0;

  /**
   * @brief Styles a set of polyline features.
   *
   * @param asyncSystem The async system.
   * @param model The glTF model containing the features and metadata
   * information.
   * @param featureIds The feature IDs of the polylines to style.
   * @param polylines The geometry of the polylines to style. This will be the
   * same size as `featureIds`, and each polyline corresponds to the feature ID
   * at the same index.
   * @returns A future that resolves to a vector of optional LineStyle
   * objects, one for each polyline feature. The vector should be the same size
   * as `featureIds` and `polylines`. If `std::nullopt` is provided for a
   * feature, the feature will use the default style. If the returned vector is
   * empty or does not match the size of `featureIds` and `polylines`, the
   * default style will be used for all features.
   */
  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::LineStyle>>>
  onStylePolylines(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>&
          polylines) = 0;

  /**
   * @brief Styles a set of polygon features.
   *
   * @param asyncSystem The async system.
   * @param model The glTF model containing the features and metadata
   * information.
   * @param featureIds The feature IDs of the polygons to style.
   * @param polygons The geometry of the polygons to style. This will be the
   * same size as `featureIds`, and each polygon corresponds to the feature ID
   * at the same index.
   * @returns A future that resolves to a vector of optional PolygonStyle
   * objects, one for each polygon feature. The vector should be the same size
   * as `featureIds` and `polygons`. If `std::nullopt` is provided for a
   * feature, the feature will use the default style. If the returned vector is
   * empty or does not match the size of `featureIds` and `polygons`, the
   * default style will be used for all features.
   */
  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::PolygonStyle>>>
  onStylePolygons(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>&
          polygons) = 0;
};

} // namespace CesiumVectorOverlays