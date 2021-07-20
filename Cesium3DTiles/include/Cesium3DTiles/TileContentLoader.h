#pragma once

#include "Cesium3DTiles/TileContentLoadInput.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"

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
   * @param asyncSystem The async system to use during the loading.
   * @param pAssetAccessor The asset accessor to use if needed.
   * @param input The {@link TileContentLoadInput}
   * @return The {@link TileContentLoadResult}. This may be the `nullptr` if the
   * tile content could not be loaded.
   */
  virtual CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const TileContentLoadInput& input) = 0;
};
} // namespace Cesium3DTiles
