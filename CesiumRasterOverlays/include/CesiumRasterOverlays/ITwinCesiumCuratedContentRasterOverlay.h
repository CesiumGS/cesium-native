#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/Library.h>

#include <functional>
#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief A {@link RasterOverlay} that obtains imagery data from the iTwin Cesium Curated Content API.
 */
class CESIUMRASTEROVERLAYS_API ITwinCesiumCuratedContentRasterOverlay final
    : public IonRasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * The tiles that are provided by this instance will contain
   * imagery data that was obtained from the iTwin CCC asset
   * with the given ID, accessed with the given access token.
   *
   * @param name The user-given name of this overlay layer.
   * @param assetID The asset ID.
   * @param iTwinAccessToken The access token.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  ITwinCesiumCuratedContentRasterOverlay(
      const std::string& name,
      int64_t assetID,
      const std::string& iTwinAccessToken,
      const RasterOverlayOptions& overlayOptions = {});
};

} // namespace CesiumRasterOverlays
