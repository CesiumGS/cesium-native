#include <CesiumRasterOverlays/ITwinCesiumCuratedContentRasterOverlay.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <fmt/format.h>

#include <cstdint>
#include <string>

namespace CesiumRasterOverlays {
ITwinCesiumCuratedContentRasterOverlay::ITwinCesiumCuratedContentRasterOverlay(
    const std::string& name,
    int64_t assetID,
    const std::string& iTwinAccessToken,
    const RasterOverlayOptions& overlayOptions)
    : IonRasterOverlay(
          name,
          fmt::format(
              "https://api.bentley.com/curated-content/cesium/{}/tiles",
              assetID),
          iTwinAccessToken,
          true,
          overlayOptions) {}
} // namespace CesiumRasterOverlays
