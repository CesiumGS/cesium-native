#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/TileContentLoader.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TileRefine.h"

#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>
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
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const TileContentLoadInput& input) override;
};

} // namespace Cesium3DTilesSelection
