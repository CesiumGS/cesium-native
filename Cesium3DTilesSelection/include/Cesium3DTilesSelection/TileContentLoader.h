#pragma once

#include "Cesium3DTilesSelection/TileContentLoadInput.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "CesiumAsync/AsyncSystem.h"

namespace Cesium3DTilesSelection {
/**
 * @brief Can create a {@link TileContentLoadResult} from a
 * {@link TileContentLoadInput}.
 */
class CESIUM3DTILESSELECTION_API TileContentLoader {

public:
  virtual ~TileContentLoader() = default;

  /**
   * @brief Loads the tile content from the given input.
   *
   * @param input The {@link TileContentLoadInput}
   * @return The {@link TileContentLoadResult}. This may be the `nullptr` if the
   * tile content could not be loaded.
   */
  virtual CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const TileContentLoadInput& input) = 0;
};
} // namespace Cesium3DTilesSelection
