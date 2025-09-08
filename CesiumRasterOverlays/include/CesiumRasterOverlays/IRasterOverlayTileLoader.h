#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <cstdint>
#include <vector>

namespace CesiumRasterOverlays {

class RasterOverlayTile;

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

class IRasterOverlayTileLoader {
public:
  /**
   * @brief Loads the image for a tile.
   *
   * @param overlayTile The overlay tile for which to load the image.
   * @return A future that resolves to the image or error information.
   */
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) = 0;

  virtual void addReference() const = 0;
  virtual void releaseReference() const = 0;
};

} // namespace CesiumRasterOverlays
