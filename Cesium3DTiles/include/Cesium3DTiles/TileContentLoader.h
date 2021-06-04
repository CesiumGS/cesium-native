#pragma once

#include "Cesium3DTiles/TileContentLoadInput.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "CesiumAsync/AsyncSystem.h"

namespace Cesium3DTiles {
/**
 * @brief Can create a {@link TileContentLoadResult} from a
 * {@link TileContentLoadInput}.
 */
class CESIUM3DTILES_API TileContentLoader {

public:
  virtual ~TileContentLoader() = default;

  /**
   * @brief Loads the tile content from the given input.
   *
   * @param input The {@link TileContentLoadInput}
   * @return The {@link TileContentLoadResult}. This may be the `nullptr` if the
   * tile content could not be loaded.
   */
  virtual CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(
    const CesiumAsync::AsyncSystem& asyncSystem, 
    const TileContentLoadInput& input) = 0;
};
} // namespace Cesium3DTiles
