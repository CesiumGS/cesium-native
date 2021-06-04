#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "CesiumAsync/AsyncSystem.h"
#include <memory>
#include <spdlog/fwd.h>
#include <string>

namespace Cesium3DTiles {

class Tileset;

/**
 * @brief Creates a {@link TileContentLoadResult} from CMPT data.
 */
class CESIUM3DTILES_API CompositeContent final : public TileContentLoader {
public:
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) override;
};

} // namespace Cesium3DTiles
