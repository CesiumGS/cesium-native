#pragma once

#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/VectorStyle.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>

namespace CesiumVectorOverlays {

/**
 * @brief A set of options for configuring a GeoJsonDocumentRasterOverlay.
 */
struct GeoJsonDocumentRasterOverlayOptions {
  /**
   * @brief The default style to use when no style is otherwise specified on a
   * \ref CesiumVectorData::GeoJsonObject.
   */
  CesiumVectorData::VectorStyle defaultStyle;

  /**
   * @brief The ellipsoid to use for this overlay.
   */
  CesiumGeospatial::Ellipsoid ellipsoid;

  /**
   * @brief The number of mip levels to generate.
   */
  uint32_t mipLevels = 0;
};

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::GeoJsonDocument.
 *
 * ## Threading and updates
 *
 * All methods of this class must be called from the main thread.
 *
 * When a tile provider is created for this overlay (when it's added to a
 * tileset), it builds a spatial index of the document and copies every style
 * it needs, resolving each geometry's style from the geometry, its feature,
 * its collection, or \ref GeoJsonDocumentRasterOverlayOptions::defaultStyle.
 * Tiles are then rasterized on worker threads from that snapshot, so:
 *
 * - The document's *styles* may be changed in place, from the main thread, at
 *   any time. The changes become visible after calling \ref refreshStyles.
 * - The document's *geometry* must not be changed while the overlay uses the
 *   document. To change geometry, build a new document and pass it to
 *   \ref setDocument.
 *
 * After \ref refreshStyles, \ref setDocument, or \ref setDefaultStyle, the
 * affected tiles are re-rasterized. Tiles that are already shown keep their
 * current image until the replacement is ready, so the overlay does not
 * flicker. The same overlay may be added to several tilesets; each one is
 * updated.
 */
class CESIUMVECTOROVERLAYS_API GeoJsonDocumentRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {

public:
  /**
   * @brief Creates a new GeoJsonDocumentRasterOverlay.
   *
   * @param asyncSystem The async system to use.
   * @param name The user-given name of this polygon layer.
   * @param document The GeoJSON document to use for the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  GeoJsonDocumentRasterOverlay(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& name,
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document,
      const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  /**
   * @brief Creates a new GeoJsonDocumentRasterOverlay from a future.
   *
   * @param name The user-given name of this polygon layer.
   * @param documentFuture A future resolving into a GeoJSON document to use for
   * the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  GeoJsonDocumentRasterOverlay(
      const std::string& name,
      CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>&&
          documentFuture,
      const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});

  virtual ~GeoJsonDocumentRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
          parameters) const override;

  /**
   * @brief Replaces the document rendered by this overlay.
   *
   * The spatial index is rebuilt on a worker thread. When it is ready, the
   * tiles that overlapped the old or the new document's contents are
   * re-rasterized. Until then, the old document is still rendered, so it must
   * stay unchanged. The overlay keeps a reference to it as long as necessary.
   *
   * If this is called before the document passed to the constructor has
   * loaded, this document is used instead.
   *
   * @param pDocument The new document. Must not be `nullptr`. Its geometry
   * must not be changed afterward; its styles may be changed in place followed
   * by a call to \ref refreshStyles.
   */
  void
  setDocument(const std::shared_ptr<const CesiumVectorData::GeoJsonDocument>&
                  pDocument);

  /**
   * @brief Re-reads the styles of the current document after they were
   * changed in place.
   *
   * The styles are copied on the main thread during the next update of each
   * tileset that uses this overlay, so styles may keep being changed in place
   * from the main thread without affecting tiles that are being rasterized on
   * worker threads. Once the copy has been indexed, only the tiles that
   * overlap geometry whose effective style changed are re-rasterized,
   * including any area that a wider line or larger point now covers.
   */
  void refreshStyles();

  /**
   * @brief Gets the style used for geometry that has no style of its own and
   * no style inherited from a feature or collection.
   */
  const CesiumVectorData::VectorStyle& getDefaultStyle() const noexcept;

  /**
   * @brief Sets the style used for geometry that has no style of its own and
   * no style inherited from a feature or collection.
   *
   * The affected tiles are re-rasterized, like \ref refreshStyles.
   *
   * @param style The new default style.
   */
  void setDefaultStyle(const CesiumVectorData::VectorStyle& style);

  /**
   * @brief State shared between this overlay and the tile providers created
   * from it. This is an implementation detail.
   *
   * @private
   */
  struct SharedState;

private:
  std::shared_ptr<SharedState> _pState;
  GeoJsonDocumentRasterOverlayOptions _options;
};
} // namespace CesiumVectorOverlays
