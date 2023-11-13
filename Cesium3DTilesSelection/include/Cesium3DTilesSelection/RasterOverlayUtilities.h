#pragma once

#include "Library.h"
#include "RasterOverlayDetails.h"

#include <CesiumGeospatial/Projection.h>

#include <glm/fwd.hpp>

#include <optional>
#include <vector>

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTilesSelection {

struct CESIUM3DTILESSELECTION_API RasterOverlayUtilities {
  /**
   * @brief Creates texture coordinates for mapping {@link RasterOverlay} tiles
   * to {@link Tileset} tiles.
   *
   * Generates new texture coordinates for the `gltf` using the given
   * `projections`. The first new texture coordinate (`u` or `s`) will be 0.0 at
   * the `minimumX` of the given `rectangle` and 1.0 at the `maximumX`. The
   * second texture coordinate (`v` or `t`) will be 0.0 at the `minimumY` of
   * the given `rectangle` and 1.0 at the `maximumY`.
   *
   * Coordinate values for vertices in between these extremes are determined by
   * projecting the vertex position with the `projection` and then computing the
   * fractional distance of that projected position between the minimum and
   * maximum.
   *
   * Projected positions that fall outside the `globeRectangle` will be clamped
   * to the edges, so the coordinate values will never be less then 0.0 or
   * greater than 1.0.
   *
   * These texture coordinates are stored in the provided glTF, and a new
   * primitive attribute named `_CESIUMOVERLAY_n` is added to each primitive,
   * where `n` starts with the `firstTextureCoordinateID` passed to this
   * function and increases with each projection.
   *
   * @param gltf The glTF model.
   * @param modelToEcefTransform The transformation of this glTF to ECEF
   * coordinates.
   * @param firstTextureCoordinateID The texture coordinate ID of the first
   * projection.
   * @param globeRectangle The rectangle that all projected vertex positions are
   * expected to lie within. If this parameter is std::nullopt, it is computed
   * from the vertices.
   * @param projections The projections for which to generate texture
   * coordinates. There is a linear relationship between the coordinates of this
   * projection and the generated texture coordinates.
   * @return The details of the generated texture coordinates.
   */
  static std::optional<RasterOverlayDetails>
  createRasterOverlayTextureCoordinates(
      CesiumGltf::Model& gltf,
      const glm::dmat4& modelToEcefTransform,
      int32_t firstTextureCoordinateID,
      const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
      std::vector<CesiumGeospatial::Projection>&& projections);
};

} // namespace Cesium3DTilesSelection
