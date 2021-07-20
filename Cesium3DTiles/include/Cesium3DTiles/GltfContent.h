#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumGltf/GltfReader.h"
#include <cstddef>
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <spdlog/fwd.h>

namespace Cesium3DTiles {

class Tileset;

/**
 * @brief Creates {@link TileContentLoadResult} from glTF data.
 */
class CESIUM3DTILES_API GltfContent final : public TileContentLoader {
public:
  /**
   * @copydoc TileContentLoader::load
   *
   * The result will only contain the `model`. Other fields will be
   * empty or have default values.
   */
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const TileContentLoadInput& input) override;

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
   * @brief Creates texture coordinates for raster tiles that are mapped to 3D
   * tiles.
   *
   * This is not supposed to be called by clients.
   *
   * It will be called for all {@link RasterMappedTo3DTile} objects of a
   * {@link Tile}, and extend the accessors of the given glTF model with
   * accessors that contain the texture coordinate sets for different
   * projections. Further details are not specified here.
   *
   * @param gltf The glTF model.
   * @param textureCoordinateID The texture coordinate ID.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param rectangle The {@link CesiumGeometry::Rectangle}.
   * @return The bounding region.
   */
  static CesiumGeospatial::BoundingRegion createRasterOverlayTextureCoordinates(
      CesiumGltf::Model& gltf,
      uint32_t textureCoordinateID,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& rectangle);

private:
  static CesiumGltf::GltfReader _gltfReader;
};

} // namespace Cesium3DTiles
