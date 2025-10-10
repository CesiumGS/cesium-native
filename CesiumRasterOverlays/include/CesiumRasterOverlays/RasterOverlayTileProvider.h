#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>

#include <spdlog/fwd.h>

#include <optional>

namespace CesiumUtility {
class CreditReferencer;
}

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayTile;
class IPrepareRasterOverlayRendererResources;

/**
 * @brief Summarizes the result of loading an image of a {@link RasterOverlay}.
 */
struct CESIUMRASTEROVERLAYS_API LoadedRasterOverlayImage {
  /**
   * @brief The loaded image.
   *
   * This will be nullptr if the loading failed. In this case, the `errors`
   * vector will contain the corresponding error messages.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> pImage{nullptr};

  /**
   * @brief The projected rectangle defining the bounds of this image.
   *
   * The rectangle extends from the left side of the leftmost pixel to the
   * right side of the rightmost pixel, and similar for the vertical direction.
   */
  CesiumGeometry::Rectangle rectangle{};

  /**
   * @brief The {@link CesiumUtility::Credit} objects that decribe the attributions that
   * are required when using the image.
   */
  std::vector<CesiumUtility::Credit> credits{};

  /**
   * @brief Errors and warnings from loading the image.
   *
   * If the image was loaded successfully, there should not be any errors (but
   * there may be warnings).
   */
  CesiumUtility::ErrorList errorList{};

  /**
   * @brief Whether more detailed data, beyond this image, is available within
   * the bounds of this image.
   */
  bool moreDetailAvailable = false;

  /**
   * @brief Returns the size of this `LoadedRasterOverlayImage` in bytes.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(LoadedRasterOverlayImage));
    accum += int64_t(this->credits.capacity() * sizeof(CesiumUtility::Credit));
    if (this->pImage) {
      accum += this->pImage->getSizeBytes();
    }
    return accum;
  }
};

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
   * {@link LoadedRasterOverlayImage::credits} property.
   */
  std::vector<CesiumUtility::Credit> credits{};

  /**
   * @brief Whether more detailed data, beyond this image, is available within
   * the bounds of this image.
   */
  bool moreDetailAvailable = true;

  /**
   * @brief Whether empty (zero length) images are accepted as a valid
   * response.
   *
   * If true, an otherwise valid response with zero length will be accepted as
   * a valid 0x0 image. If false, such a response will be reported as an
   * error.
   *
   * {@link ActivatedRasterOverlay::loadTile} and
   * {@link ActivatedRasterOverlay::loadTileThrottled} will treat such an
   * image as "failed" and use the quadtree parent (or ancestor) image
   * instead, but will not report any error.
   *
   * This flag should only be set to `true` when the tile source uses a
   * zero-length response as an indication that this tile is - as expected -
   * not available.
   */
  bool allowEmptyImages = false;
};

class RasterOverlayTileProvider;

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
   * @brief Creates a new instance.
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param externals The external interfaces for use by the raster overlay.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param coverageRectangle The rectangle that bounds all the area covered by
   * this overlay, expressed in projected coordinates.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const RasterOverlayExternals& externals,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& coverageRectangle) noexcept;

  /**
   * @brief Creates a new instance.
   * @deprecated Use the overload that takes a \ref RasterOverlayExternals
   * instead.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& coverageRectangle) noexcept;

  /** @brief Default destructor. */
  virtual ~RasterOverlayTileProvider() noexcept;

  /**
   * @brief A future that resolves when this RasterOverlayTileProvider has been
   * destroyed (i.e. its destructor has been called) and all async operations
   * that it was executing have completed.
   */
  CesiumAsync::SharedFuture<void>& getAsyncDestructionCompleteEvent();

  /**
   * @brief Returns the {@link RasterOverlay} that created this instance.
   */
  RasterOverlay& getOwner() noexcept;

  /** @copydoc getOwner */
  const RasterOverlay& getOwner() const noexcept;

  /**
   * @brief Get the external interfaces for use by the tile provider.
   */
  const RasterOverlayExternals& getExternals() const noexcept;

  /**
   * @brief Get the system to use for asychronous requests and threaded work.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>&
  getAssetAccessor() const noexcept;

  /**
   * @brief Get the credit system that receives credits from this tile provider.
   */
  const std::shared_ptr<CesiumUtility::CreditSystem>&
  getCreditSystem() const noexcept;

  /**
   * @brief Gets the async system used to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept;

  /**
   * @brief Gets the interface used to prepare raster overlay images for
   * rendering.
   */
  const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
  getPrepareRendererResources() const noexcept;

  /**
   * @brief Gets the logger to which to send messages about the tile provider
   * and tiles.
   */
  const std::shared_ptr<spdlog::logger>& getLogger() const noexcept;

  /**
   * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
   */
  const CesiumGeospatial::Projection& getProjection() const noexcept;

  /**
   * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this
   * instance.
   */
  const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept;

  /**
   * @brief Get the per-TileProvider {@link CesiumUtility::Credit} if one exists.
   * @deprecated Implement {@link addCredits} instead.
   */
  [[deprecated(
      "Use addCredits instead.")]] const std::optional<CesiumUtility::Credit>&
  getCredit() const noexcept;

  /**
   * @brief Loads the image for a tile.
   *
   * @param overlayTile The overlay tile for which to load the image.
   * @return A future that resolves to the image or error information.
   */
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) = 0;

  /**
   * @brief Adds this tile provider's credits to a credit referencer.
   *
   * The added credits will be displayed whenever the @ref RasterOverlay that
   * owns this tile provider is displayed. To show tile-specific credits, add
   * them to @ref LoadedRasterOverlayImage::credits in the instance returned by
   * @ref loadTileImage.
   *
   * @param creditReferencer The credit referencer to which to add credits.
   */
  virtual void
  addCredits(CesiumUtility::CreditReferencer& creditReferencer) noexcept;

protected:
  /**
   * @brief Loads an image from a URL and optionally some request headers.
   *
   * This is a useful helper function for implementing {@link loadTileImage}.
   *
   * @param url The URL.
   * @param headers The request headers.
   * @param options Additional options for the load process.
   * @return A future that resolves to the image or error information.
   */
  CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImageFromUrl(
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      LoadTileImageFromUrlOptions&& options = {}) const;

private:
  struct DestructionCompleteDetails {
    CesiumAsync::Promise<void> promise;
    CesiumAsync::SharedFuture<void> future;
  };

  CesiumUtility::IntrusivePointer<RasterOverlay> _pOwner;
  RasterOverlayExternals _externals;
  std::optional<CesiumUtility::Credit> _credit;
  CesiumGeospatial::Projection _projection;
  CesiumGeometry::Rectangle _coverageRectangle;
  std::optional<DestructionCompleteDetails> _destructionCompleteDetails;
};
} // namespace CesiumRasterOverlays
