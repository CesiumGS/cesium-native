#pragma once

#include "Cesium3DTilesSelection/TileContentLoadInput.h"
#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/TileContentLoader.h"
#include "CesiumAsync/Future.h"
#include "CesiumAsync/IAssetRequest.h"

#include <memory>

namespace Cesium3DTilesSelection {

class AvailabilitySubtreeContent : public TileContentLoader {
public:
  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  load(const TileContentLoadInput& input) override;
};

} // namespace Cesium3DTilesSelection