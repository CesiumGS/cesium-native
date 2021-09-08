#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/TileContentLoader.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TileRefine.h"
#include <cstddef>
#include <memory>
#include <spdlog/fwd.h>
#include <string>

namespace Cesium3DTilesSelection {

class Tileset;

/**
 * @brief Creates a {@link TileContentLoadResult} from B3DM data.
 */
class CESIUM3DTILESSELECTION_API Batched3DModelContent final
    : public TileContentLoader {
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
      const std::vector<std::pair<std::string, std::string>>& requestHeaders,
      const TileContentLoadInput& input) override;

  /**
   * @brief Create a {@link TileContentLoadResult} from the given data.
   *
   * @param pLogger Only used for logging
   * @param url The URL, only used for logging
   * @param data The actual B3DM data
   * @return The {@link TileContentLoadResult}, or `nullptr` if the
   * data cannot be loaded. The returned result will *only* contain
   * the `model`. All other properties will be uninitialized.
   */
  static std::unique_ptr<TileContentLoadResult> load(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& url,
      const gsl::span<const std::byte>& data);
};

} // namespace Cesium3DTilesSelection
