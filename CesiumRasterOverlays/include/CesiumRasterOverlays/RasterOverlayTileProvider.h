#pragma once

#include "Library.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>

#include <spdlog/fwd.h>

#include <cassert>
#include <optional>

namespace CesiumUtility {
struct Credit;
} // namespace CesiumUtility

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayTile;
class IPrepareRasterOverlayRendererResources;

struct RasterLoadResult {
  std::optional<CesiumGltf::ImageCesium> image{};
  CesiumGeometry::Rectangle rectangle = {};
  std::vector<CesiumUtility::Credit> credits = {};
  std::vector<std::string> errors{};
  std::vector<std::string> warnings{};
  bool moreDetailAvailable = false;

  std::vector<CesiumAsync::RequestData> missingRequests = {};

  RasterOverlayTile::LoadState state = RasterOverlayTile::LoadState::Unloaded;

  void* pRendererResources = nullptr;

  CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile = {};
};

using RasterProcessingCallback =
    std::function<CesiumAsync::Future<RasterLoadResult>(
        RasterOverlayTile&,
        RasterOverlayTileProvider*,
        const CesiumAsync::UrlResponseDataMap&)>;

/**
 * @brief Options for {@link RasterOverlayTileProvider::loadTileImageFromUrl}.
 */
struct LoadTileImageFromUrlOptions {
  /**
   * @brief The rectangle definining the bounds of the image being loaded,
   * expressed in the {@link RasterOverlayTileProvider}'s projection.
   */
  CesiumGeometry::Rectangle rectangle{};

  /**
   * @brief The credits to display with this tile.
   *
   * This property is copied verbatim to the
   * {@link RasterLoadResult::credits} property.
   */
  std::vector<CesiumUtility::Credit> credits{};

  /**
   * @brief Whether more detailed data, beyond this image, is available within
   * the bounds of this image.
   */
  bool moreDetailAvailable = true;
};

/**
 * @brief Provides individual tiles for a {@link RasterOverlay} on demand.
 *
 * Instances of this class must be allocated on the heap, and their lifetimes
 * must be managed with {@link CesiumUtility::IntrusivePointer}.
 */
class CESIUMRASTEROVERLAYS_API RasterOverlayTileProvider
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          RasterOverlayTileProvider> {
public:
  /**
   * Constructs a placeholder tile provider.
   *
   * @see RasterOverlayTileProvider::isPlaceholder
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this raster overlay.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>&
          pAssetAccessor) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this raster overlay.
   * @param credit The {@link Credit} for this tile provider, if it exists.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param coverageRectangle The rectangle that bounds all the area covered by
   * this overlay, expressed in projected coordinates.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& coverageRectangle) noexcept;

  /** @brief Default destructor. */
  virtual ~RasterOverlayTileProvider() noexcept;

  /**
   * @brief Returns whether this is a placeholder.
   *
   * For many types of {@link RasterOverlay}, we can't create a functioning
   * `RasterOverlayTileProvider` right away. For example, we may not know the
   * bounds of the overlay, or what projection it uses, until after we've
   * (asynchronously) loaded a metadata service that gives us this information.
   *
   * So until that real `RasterOverlayTileProvider` becomes available, we use
   * a placeholder. When {@link RasterOverlayTileProvider::getTile} is invoked
   * on a placeholder, it returns a {@link RasterOverlayTile} that is also
   * a placeholder. And whenever we see a placeholder `RasterOverTile` in
   * {@link Tile::update}, we check if the corresponding `RasterOverlay` is
   * ready yet. Once it's ready, we remove the placeholder tile and replace
   * it with the real tiles.
   *
   * So the placeholder system gives us a way to defer the mapping of raster
   * overlay tiles to geometry tiles until that mapping can be determined.
   */
  bool isPlaceholder() const noexcept { return this->_pPlaceholder != nullptr; }

  /**
   * @brief Returns the {@link RasterOverlay} that created this instance.
   */
  RasterOverlay& getOwner() noexcept { return *this->_pOwner; }

  /** @copydoc getOwner */
  const RasterOverlay& getOwner() const noexcept { return *this->_pOwner; }

  /**
   * @brief Get the system to use for asychronous requests and threaded work.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>&
  getAssetAccessor() const noexcept {
    return this->_pAssetAccessor;
  }

  /**
   * @brief Gets the async system used to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /**
   * @brief Gets the interface used to prepare raster overlay images for
   * rendering.
   */
  const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
  getPrepareRendererResources() const noexcept {
    return this->_pPrepareRendererResources;
  }

  /**
   * @brief Gets the logger to which to send messages about the tile provider
   * and tiles.
   */
  const std::shared_ptr<spdlog::logger>& getLogger() const noexcept {
    return this->_pLogger;
  }

  /**
   * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
   */
  const CesiumGeospatial::Projection& getProjection() const noexcept {
    return this->_projection;
  }

  /**
   * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this
   * instance.
   */
  const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept {
    return this->_coverageRectangle;
  }

  /**
   * @brief Returns a new {@link RasterOverlayTile} with the given
   * specifications.
   *
   * The returned tile will not start loading immediately. To start loading,
   * call {@link RasterOverlayTileProvider::loadTile} or
   * {@link RasterOverlayTileProvider::loadTileThrottled}.
   *
   * @param rectangle The rectangle that the returned image must cover. It is
   * allowed to cover a slightly larger rectangle in order to maintain pixel
   * alignment. It may also cover a smaller rectangle when the overlay itself
   * does not cover the entire rectangle.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link RasterOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @return The tile.
   */
  CesiumUtility::IntrusivePointer<RasterOverlayTile> getTile(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& targetScreenPixels);

  /**
   * @brief Gets the number of bytes of tile data that are currently loaded.
   */
  int64_t getTileDataBytes() const noexcept { return this->_tileDataBytes; }

  /**
   * @brief Incremement number of bytes loaded from outside caller
   *
   * @param bytes Number of bytes to add to our count
   */
  void incrementTileDataBytes(CesiumGltf::ImageCesium& imageCesium) noexcept {
    // If the image size hasn't been overridden, store the pixelData
    // size now. We'll add this number to our total memory usage now,
    // and remove it when the tile is later unloaded, and we must use
    // the same size in each case.
    if (imageCesium.sizeBytes < 0) {
      imageCesium.sizeBytes = int64_t(imageCesium.pixelData.size());
    }

    _tileDataBytes += imageCesium.sizeBytes;
  }

  /**
   * @brief Removes a no-longer-referenced tile from this provider's cache and
   * deletes it.
   *
   * This function is not supposed to be called by client. Calling this method
   * in a tile with a reference count greater than 0 will result in undefined
   * behavior.
   *
   * @param pTile The tile, which must have no oustanding references.
   */
  void removeTile(RasterOverlayTile* pTile) noexcept;

  /**
   * @brief Get the per-TileProvider {@link Credit} if one exists.
   */
  const std::optional<CesiumUtility::Credit>& getCredit() const noexcept {
    return _credit;
  }

  /**
   * @brief Loads a tile immediately, without throttling requests.
   *
   * If the tile is not in the `RasterOverlayTile::LoadState::Unloading`
   * state, this method returns without doing anything. Otherwise, it puts the
   * tile into the `RasterOverlayTile::LoadState::Loading` state and
   * begins the asynchronous process to load the tile. When the process
   * completes, the tile will be in the
   * `RasterOverlayTile::LoadState::Loaded` or
   * `RasterOverlayTile::LoadState::Failed` state.
   *
   * Calling this method on many tiles at once can result in very slow
   * performance. Consider using {@link loadTileThrottled} instead.
   *
   * @param tile The tile to load
   * @param responsesByUrl Content responses already fetched by caller
   * @param rasterCallback Loader callback to execute
   */
  CesiumAsync::Future<RasterLoadResult> loadTile(
      RasterOverlayTile& tile,
      const CesiumAsync::UrlResponseDataMap& responsesByUrl,
      RasterProcessingCallback rasterCallback);

  /**
   * @brief Loads a tile
   *
   * It begins the asynchronous process to load the tile, and returns true. When
   * the process completes, the tile will be in the
   * `RasterOverlayTile::LoadState::Loaded` or
   * `RasterOverlayTile::LoadState::Failed` state.
   *
   * @param tile The tile to load
   * @param responsesByUrl Content responses already fetched by caller
   * @param rasterCallback Loader callback to execute
   * @returns RasterLoadResult indicating what happened
   */
  CesiumAsync::Future<RasterLoadResult> loadTileThrottled(
      RasterOverlayTile& tile,
      const CesiumAsync::UrlResponseDataMap& responsesByUrl,
      RasterProcessingCallback rasterCallback);

  /**
   * @brief Gets the work needed to load a tile
   *
   * @param tile The tile to load
   * @param outRequest Content request needed
   * @param outCallback Loader callback to execute later
   */
  void getLoadTileThrottledWork(
      const RasterOverlayTile& tile,
      CesiumAsync::RequestData& outRequest,
      RasterProcessingCallback& outCallback);

protected:
  /**
   * @brief Loads the image for a tile.
   *
   * @param overlayTile The overlay tile to load the image.
   * @param responsesByUrl Content responses already fetched by caller
   * @return A future containing a RasterLoadResult
   */
  virtual CesiumAsync::Future<RasterLoadResult> loadTileImage(
      const RasterOverlayTile& overlayTile,
      const CesiumAsync::UrlResponseDataMap& responsesByUrl) = 0;

  /**
   * @brief Get the work needed to loads the image for a tile.
   *
   * @param overlayTile The overlay tile to load the image.
   * @param outRequest Content request needed
   * @param outCallback Loader callback to execute later
   */
  virtual void getLoadTileImageWork(
      const RasterOverlayTile& overlayTile,
      CesiumAsync::RequestData& outRequest,
      RasterProcessingCallback& outCallback) = 0;

  /**
   * @brief Loads an image from a URL and optionally some request headers.
   *
   * This is a useful helper function for implementing {@link loadTileImage}.
   *
   * @param url The URL used to fetch image data
   * @param statusCode HTTP code returned from content fetch
   * @param data Bytes of url content response
   * @param options Additional options for the load process
   * @return A future containing a RasterLoadResult
   */
  CesiumAsync::Future<RasterLoadResult> loadTileImageFromUrl(
      const std::string& url,
      const gsl::span<const std::byte>& data,
      LoadTileImageFromUrlOptions&& options = {}) const;

private:
  CesiumAsync::Future<RasterLoadResult> doLoad(
      RasterOverlayTile& tile,
      const CesiumAsync::UrlResponseDataMap& responsesByUrl,
      RasterProcessingCallback rasterCallback);

private:
  CesiumUtility::IntrusivePointer<RasterOverlay> _pOwner;
  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::optional<CesiumUtility::Credit> _credit;
  std::shared_ptr<IPrepareRasterOverlayRendererResources>
      _pPrepareRendererResources;
  std::shared_ptr<spdlog::logger> _pLogger;
  CesiumGeospatial::Projection _projection;
  CesiumGeometry::Rectangle _coverageRectangle;
  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pPlaceholder;
  int64_t _tileDataBytes;
  CESIUM_TRACE_DECLARE_TRACK_SET(
      _loadingSlots,
      "Raster Overlay Tile Loading Slot");

  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace CesiumRasterOverlays
