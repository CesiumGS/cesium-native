#include <CesiumRasterOverlays/ITwinCesiumCuratedContentRasterOverlay.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <fmt/format.h>

#include <cstdint>
#include <memory>

namespace CesiumRasterOverlays {
class ITwinCesiumCuratedContentEndpointResource : public EndpointResource {
public:
  virtual std::string getUrl(
      int64_t ionAssetID,
      const std::string& /*ionAccessToken*/,
      const std::string& /*ionAssetEndpointUrl*/) const override {
    return fmt::format(
        "https://api.bentley.com/curated-content/cesium/{}/tiles",
        ionAssetID);
  }

  virtual bool needsAuthHeaderOnInitialRequest() const override { return true; }
};

ITwinCesiumCuratedContentRasterOverlay::ITwinCesiumCuratedContentRasterOverlay(
    const std::string& name,
    int64_t assetID,
    const std::string& iTwinAccessToken,
    const RasterOverlayOptions& overlayOptions)
    : IonRasterOverlay(
          name,
          assetID,
          iTwinAccessToken,
          std::make_unique<ITwinCesiumCuratedContentEndpointResource>(),
          overlayOptions,
          {}) {}
} // namespace CesiumRasterOverlays
