#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "TileContentLoadResult.h"
#include "TileContentLoader.h"
#include "TileID.h"
#include "TileRefine.h"

#include <CesiumGltf/GltfReader.h>

#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <optional>

namespace CesiumGeospatial {
class GlobeRectangle;
}

namespace Cesium3DTilesSelection {

class Tileset;

/**
 * @brief Creates {@link TileContentLoadResult} from glTF data.
 */
class CESIUM3DTILESSELECTION_API GltfContent final : public TileContentLoader {
public:
  /**
   * @copydoc TileContentLoader::load
   *
   * The result will only contain the `model`. Other fields will be
   * empty or have default values.
   */
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const TileContentLoadInput& input) override;

  /**
   * @brief Create a {@link TileContentLoadResult} from the given data.
   *
   * (Only public to be called from `Batched3DModelContent`)
   *
   * @param pLogger Only used for logging
   * @param url The URL, only used for logging
   * @param data The actual glTF data
   * @return The {@link TileContentLoadResult}
   */
  static std::unique_ptr<TileContentLoadResult> load(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& url,
      const gsl::span<const std::byte>& data);

  /**
   * @brief Creates texture coordinates for mapping {@link RasterOverlay} tiles
   * to {@link Tileset} tiles.
   *
   * Generates new texture coordinates for the `gltf` using the given
   * `projection`. The first new texture coordinate (`u` or `s`) will be 0.0 at
   * the `minimumX` of the given `rectangle` and 1.0 at the `maximumX`. The
   * second texture coordinate (`v` or `t`) will be 0.0 at the `minimumY` of
   * the given `rectangle` and 1.0 at the `maximumY`.
   *
   * Coordinate values for vertices in between these extremes are determined by
   * projecting the vertex position with the `projection` and then computing the
   * fractional distance of that projected position between the minimum and
   * maximum.
   *
   * Projected positions that fall outside the `rectangle` will be clamped to
   * the edges, so the coordinate values will never be less then 0.0 or greater
   * than 1.0.
   *
   * These texture coordinates are stored in the provided glTF, and a new
   * primitive attribute named `_CESIUMOVERLAY_n`, where `n` is the
   * `textureCoordinateID` passed to this function, is added to each primitive.
   *
   * @param gltf The glTF model.
   * @param transform The transformation of this glTF to ECEF coordinates.
   * @param textureCoordinateID The texture coordinate ID.
   * @param projection The projection. There is a linear relationship between
   * the coordinates of this projection and the generated texture coordinates.
   * @param rectangle The rectangle that all projected vertex positions are
   * expected to lie within.
   * @return The bounding region.
   */
  static std::optional<TileContentDetailsForOverlays>
  createRasterOverlayTextureCoordinates(
      CesiumGltf::Model& gltf,
      const glm::dmat4& modelToEcefTransform,
      int32_t firstTextureCoordinateID,
      const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
      std::vector<CesiumGeospatial::Projection>&& projections);

  /**
   * @brief Computes a bounding region from the vertex positions in a glTF
   * model.
   *
   * If the glTF model spans the anti-meridian, the west and east longitude
   * values will be in the usual -PI to PI range, but east will have a smaller
   * value than west.
   *
   * @param gltf The model.
   * @param transform The transform from model coordinates to ECEF coordinates.
   * @return The computed bounding region.
   */
  static CesiumGeospatial::BoundingRegion computeBoundingRegion(
      const CesiumGltf::Model& gltf,
      const glm::dmat4& transform);

private:
  static CesiumGltf::GltfReader _gltfReader;
};

} // namespace Cesium3DTilesSelection
