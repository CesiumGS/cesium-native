#include "Library.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumVectorData/VectorStyle.h>

namespace CesiumVectorOverlays {

/**
 * @brief Options for constructing a \ref VectorTilesRasterOverlay.
 */
struct VectorTilesRasterOverlayOptions {
  /**
   * @brief The default style to use for features in the vector tileset when no
   * other style is specified.
   */
  CesiumVectorData::VectorStyle defaultStyle;
  /**
   * @brief HTTP headers to attach to requests made for this tileset.
   */
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
};

/**
 * @brief A raster overlay made from rasterizing a vector tiles tileset.
 */
class CESIUMVECTOROVERLAYS_API VectorTilesRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {

public:
  /**
   * @brief Creates a new VectorTilesRasterOverlay from a URL.
   *
   * @param name The user-given name of this layer.
   * @param url The URL of the tileset to load.
   * @param vectorOptions Options to configure this
   * VectorTilesRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorTilesRasterOverlay(
      const std::string& name,
      const std::string& url,
      const VectorTilesRasterOverlayOptions& vectorOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  /**
   * @brief Creates a new VectorTilesRasterOverlay from a Cesium ion asset.
   *
   * @param name The user-given name of this layer.
   * @param ionAssetID The ion asset ID of the tileset.
   * @param ionAccessToken The ion access token to use to access the tileset.
   * @param ionAssetEndpointUrl The ion asset endpoint URL to use to access the
   * tileset.
   * @param vectorOptions Options to configure this
   * VectorTilesRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorTilesRasterOverlay(
      const std::string& name,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/",
      const VectorTilesRasterOverlayOptions& vectorOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  VectorTilesRasterOverlayOptions _vectorOptions;
  bool _useIon = false;
  std::string _url;
  int64_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
};
} // namespace CesiumVectorOverlays