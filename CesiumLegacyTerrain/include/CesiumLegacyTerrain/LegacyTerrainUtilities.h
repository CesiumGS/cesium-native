#pragma once

#include "Library.h"

#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Projection.h>

#include <optional>

namespace CesiumGeometry {
class QuadtreeTilingScheme;
}

namespace CesiumLegacyTerrain {

struct Layer;

/**
 * @brief Helper functions for working with legacy terrain layer.json /
 * quantized-mesh.
 */
class CESIUMLEGACYTERRAIN_API LegacyTerrainUtilities final {
public:
  /**
   * @brief Gets the projection specified by a layer.json.
   *
   * @param layer The layer.json.
   * @return The projection, or std::nullopt if the layer.json does not specify
   * a valid projection.
   */
  static std::optional<CesiumGeospatial::Projection>
  getProjection(const Layer& layer);

  /**
   * @brief Gets the tiling scheme specified by the layer.json.
   *
   * @param layer The layer.json.
   * @return The tiling scheme, or std::nullopt if the layer.json does not
   * specify a tiling scheme.
   */
  static std::optional<CesiumGeometry::QuadtreeTilingScheme>
  getTilingScheme(const Layer& layer);

  /**
   * @brief Gets the bounding region for the root tile.
   *
   * The rectangle will be the maximum rectangle for the terrain's projection
   * (geographic or web mercator). The heights will range from -1000.0 to
   * 9000.0.
   *
   * @param layer The layer.json.
   * @return The bounding rectangle, or std::nullopt if the bounding region
   * cannot be determined from the layer.json.
   */
  static std::optional<CesiumGeospatial::BoundingRegion>
  getRootBoundingRegion(const Layer& layer);
};

} // namespace CesiumLegacyTerrain
