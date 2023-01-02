#pragma once

#include <CesiumUtility/IntrusivePointer.h>

#include <memory>
#include <string>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesSelection {

class RasterOverlay;

/**
 * @brief The type of load that failed in {@link TilesetLoadFailureDetails}.
 */
enum class RasterOverlayLoadType {
  /**
   * @brief An unknown load error.
   */
  Unknown,

  /**
   * @brief A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * @brief An initial load needed to create the overlay's tile provider.
   */
  TileProvider
};

class RasterOverlayLoadFailureDetails {
public:
  /**
   * @brief The type of request that failed to load.
   */
  RasterOverlayLoadType type = RasterOverlayLoadType::Unknown;

  /**
   * @brief The request that failed. The request itself may have succeeded, but
   * the failure occurred while processing this request.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest = nullptr;

  /**
   * @brief A human-readable explanation of what failed.
   */
  std::string message = "";
};

} // namespace Cesium3DTilesSelection
