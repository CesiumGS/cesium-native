#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileRefine.h"
#include "CesiumAsync/AsyncSystem.h"
#include <cstddef>
#include <gsl/span>
#include <memory>
#include <spdlog/fwd.h>
#include <vector>

namespace Cesium3DTiles {

class Tileset;

/**
 * @brief Creates a {@link TileContentLoadResult} from 3D Tiles external
 * `tileset.json` data.
 */
class CESIUM3DTILES_API ExternalTilesetContent final
    : public TileContentLoader {
public:
  /**
   * @copydoc TileContentLoader::load
   *
   * The result will only contain the `childTiles` and the `pNewTileContext`.
   * Other fields will be empty or have default values.
   */
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) override;

private:
  /**
   * @brief Create the {@link TileContentLoadResult} from the given input data.
   *
   * @param pLogger The logger that receives details of loading errors and
   * warnings.
   * @param tileRefine The {@link TileRefine}
   * @param url The source URL
   * @param data The raw input data
   * @return The {@link TileContentLoadResult}
   */
  static std::unique_ptr<TileContentLoadResult> load(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const glm::dmat4& tileTransform,
      TileRefine tileRefine,
      const std::string& url,
      const gsl::span<const std::byte>& data);
};

} // namespace Cesium3DTiles
