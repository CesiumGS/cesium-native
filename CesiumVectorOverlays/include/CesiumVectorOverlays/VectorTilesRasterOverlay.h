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
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::GeoJsonDocument.
 */
class CESIUMVECTOROVERLAYS_API VectorTilesRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {

public:
  /**
   * @brief Parameters for loading a vector tiles tileset from ion.
   */
  struct TilesetFromIon {
    /**
     * @brief The Ion asset ID.
     */
    uint32_t ionAssetID;
    /**
     * @brief The Ion access token.
     */
    std::string ionAccessToken;
    /**
     * @brief The Ion asset endpoint URL.
     */
    std::string ionAssetEndpointUrl = "https://api.cesium.com/";
  };

  /**
   * @brief A source for a tileset, which can be either a URL or an Ion asset.
   */
  using TilesetSource = std::variant<std::string, TilesetFromIon>;

  /**
   * @brief Creates a new VectorTilesRasterOverlay.
   *
   * @param asyncSystem The async system to use.
   * @param name The user-given name of this layer.
   * @param source The source of the tileset.
   * @param vectorOptions Options to configure this
   * VectorTilesRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorTilesRasterOverlay(
      const std::string& name,
      const TilesetSource& source,
      const VectorTilesRasterOverlayOptions& vectorOptions = {},
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  virtual ~VectorTilesRasterOverlay() override = default;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

private:
  VectorTilesRasterOverlayOptions _vectorOptions;
  TilesetSource _source;
};
} // namespace CesiumVectorOverlays