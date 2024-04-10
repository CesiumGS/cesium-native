#pragma once

#include "Library.h"

#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumQuantizedMeshTerrain/LayerSpec.h>

#include <optional>

namespace CesiumQuantizedMeshTerrain {

/**
 * @brief A quantized-mesh terrain layer.json.
 */
struct Layer : public LayerSpec {
  /**
   * @brief Gets the projection specified by this layer.json.
   *
   * @return The projection, or std::nullopt if this layer.json does not specify
   * a valid projection.
   */
  std::optional<CesiumGeospatial::Projection> getProjection() const noexcept;

  /**
   * @brief Gets the tiling scheme specified by this layer.json.
   *
   * @return The tiling scheme, or std::nullopt if this layer.json does not
   * specify a tiling scheme.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme>
  getTilingScheme() const noexcept;

  /**
   * @brief Gets the bounding region for the root tile.
   *
   * The rectangle will be the maximum rectangle for the terrain's projection
   * (geographic or web mercator). The heights will range from -1000.0 to
   * 9000.0.
   *
   * @return The bounding rectangle, or std::nullopt if the bounding region
   * cannot be determined from this layer.json.
   */
  std::optional<CesiumGeospatial::BoundingRegion>
  getRootBoundingRegion() const noexcept;
};

} // namespace CesiumQuantizedMeshTerrain
