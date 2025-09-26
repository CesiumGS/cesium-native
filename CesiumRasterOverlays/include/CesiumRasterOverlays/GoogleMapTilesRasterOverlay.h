#pragma once

#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumUtility/JsonValue.h>

#include <optional>
#include <string>

namespace CesiumRasterOverlays {

struct GoogleMapTilesExistingSession {
  std::string apiBaseUrl;
  std::string key;
  std::string session;
  std::string expiry;
  int32_t tileWidth;
  int32_t tileHeight;
  std::string imageFormat;
};

struct GoogleMapTilesNewSessionParameters {
  std::string key;
  std::string mapType{"satellite"};
  std::string language{"en-US"};
  std::string region{"US"};

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
      const GoogleMapTilesNewSessionParameters& newSessionParameters,
      const RasterOverlayOptions& overlayOptions = {});

  GoogleMapTilesRasterOverlay(
      const std::string& name,
      const GoogleMapTilesExistingSession& existingSession,
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
  CesiumAsync::Future<CreateTileProviderResult> createNewSession(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const;

  std::optional<GoogleMapTilesNewSessionParameters> _newSessionParameters;
  std::optional<GoogleMapTilesExistingSession> _existingSession;
};

} // namespace CesiumRasterOverlays
