#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

namespace Cesium3DTilesSelection {
class TilesetContentManager;

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::GeoJsonDocument.
 */
class CESIUMRASTEROVERLAYS_API EmbeddedTilesetRasterOverlayTest final
    : public CesiumRasterOverlays::RasterOverlay {

public:
  /**
   * @brief Creates a new EmbeddedTilesetRasterOverlayTest.
   *
   * @param asyncSystem The async system to use.
   * @param name The user-given name of this polygon layer.
   * @param document The GeoJSON document to use for the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  EmbeddedTilesetRasterOverlayTest(
      const std::string& name,
      const std::string& url,
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  virtual ~EmbeddedTilesetRasterOverlayTest() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  std::string _url;
};
} // namespace Cesium3DTilesSelection