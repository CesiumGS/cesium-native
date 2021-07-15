#pragma once

#include "Cesium3DTilesPipeline/BoundingVolume.h"
#include "Cesium3DTilesPipeline/Library.h"
#include "Cesium3DTilesPipeline/TileContentLoadResult.h"
#include "Cesium3DTilesPipeline/TileContentLoader.h"
#include "Cesium3DTilesPipeline/TileID.h"
#include "Cesium3DTilesPipeline/TileRefine.h"
#include <memory>
#include <spdlog/fwd.h>
#include <string>

namespace Cesium3DTilesPipeline {

class Tileset;

/**
 * @brief Creates a {@link TileContentLoadResult} from CMPT data.
 */
class CESIUM3DTILESPIPELINE_API CompositeContent final : public TileContentLoader {
public:
  std::unique_ptr<TileContentLoadResult>
  load(const TileContentLoadInput& input) override;
};

} // namespace Cesium3DTilesPipeline
