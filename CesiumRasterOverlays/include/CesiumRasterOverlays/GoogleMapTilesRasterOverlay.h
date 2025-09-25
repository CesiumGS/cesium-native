#pragma once

#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumUtility/JsonValue.h>

#include <optional>
#include <string>

namespace CesiumRasterOverlays {

/**
 * @brief Additional options for @ref GoogleMapTilesRasterOverlay.
 */
struct GoogleMapTilesOptions {
  int32_t maximumZoomLevel{28};

  std::optional<std::string> imageFormat{};
  std::optional<std::string> scale{};
  std::optional<bool> highDpi{};
  std::optional<std::string> layerTypes{};
  std::optional<CesiumUtility::JsonValue::Array> styles{};
  std::optional<bool> overlay{};
  std::string apiBaseUrl{"https://tile.googleapis.com/"};
};

class CESIUMRASTEROVERLAYS_API GoogleMapTilesRasterOverlay
    : public RasterOverlay {
public:
  GoogleMapTilesRasterOverlay(
      const std::string& name,
      const std::string& key,
      const std::string& mapType,
      const std::string& language,
      const std::string& region,
      const GoogleMapTilesOptions& googleOptions = {},
      const RasterOverlayOptions& overlayOptions = {});

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

private:
  std::string _key;
  std::string _mapType;
  std::string _language;
  std::string _region;
  GoogleMapTilesOptions _googleOptions;
};

} // namespace CesiumRasterOverlays
