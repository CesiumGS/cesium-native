#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "CesiumAsync/AsyncSystem.h"
#include <cstddef>
#include <memory>
#include <spdlog/fwd.h>
#include <string>

namespace Cesium3DTiles {

class Tileset;

/**
 * @brief Creates a {@link TileContentLoadResult} from I3DM data.
 */
class CESIUM3DTILES_API Instanced3DModelContent final : public TileContentLoader {
public:
  /**
   * @copydoc TileContentLoader::load
   *
   * The result will only contain the `model`. Other fields will be
   * empty or have default values.
   */
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) override;

private:
  /**
   * @brief Create a {@link TileContentLoadResult} from the given data.
   *
   * @param pLogger Only used for logging
   * @param url The URL, only used for logging
   * @param data The actual I3DM data
   * @return The {@link TileContentLoadResult}, or `nullptr` if the
   * data cannot be loaded. The returned result will *only* contain
   * the `model`. All other properties will be uninitialized.
   */
  static std::unique_ptr<TileContentLoadResult> load(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& url,
      const gsl::span<const std::byte>& data);
};

} // namespace Cesium3DTiles
