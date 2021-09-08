#pragma once

#include "Cesium3DTilesSelection/TileContentLoadInput.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include <string>
#include <utility>
#include <vector>

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
   * @param asyncSystem The async system to use during the loading.
   * @param pAssetAccessor The asset accessor to use if needed.
   * @param requestHeaders The request headers to be used if needed.
   * @param input The {@link TileContentLoadInput}
   * @return The {@link TileContentLoadResult}. This may be the `nullptr` if the
   * tile content could not be loaded.
   */
  virtual CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::vector<std::pair<std::string, std::string>>& requestHeaders,
      const TileContentLoadInput& input) = 0;
};
} // namespace Cesium3DTilesSelection
