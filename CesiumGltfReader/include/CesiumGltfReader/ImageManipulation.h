#pragma once

#include "CesiumGltfReader/Library.h"

#include <cstddef>
#include <cstdint>

// Forward declarations
namespace CesiumGltf {
struct ImageCesium;
}

namespace CesiumGltfReader {

/**
 * @brief Specifies a rectangle of pixels in an image.
 */
struct PixelRectangle {
  /**
   * @brief The X coordinate of the top-left corner of the rectangle.
   */
  int32_t x;

  /**
   * @brief The Y coordinate of the top-left corner of the rectangle.
   */
  int32_t y;

  /**
   * @brief The total number of pixels in the horizontal direction.
   */
  int32_t width;

  /**
   * @brief The total number of pixels in the vertical direction.
   */
  int32_t height;
};

class CESIUMGLTFREADER_API ImageManipulation {
public:
  /**
   * @brief Directly copies pixels from a source to a target, without validating
   * the provided pointers or ranges.
   *
   * @param pTarget The pointer at which to start writing pixels.
   * @param targetRowStride The number of bytes between rows in the target
   * image.
   * @param pSource The pointer at which to start reading pixels.
   * @param sourceRowStride The number of bytes between rows in the source
   * image.
   * @param sourceWidth The number of pixels to copy in the horizontal
   * direction.
   * @param sourceHeight The number of pixels to copy in the vertical direction.
   * @param bytesPerPixel The number of bytes used to represent each pixel.
   */
  static void unsafeBlitImage(
      std::byte* pTarget,
      size_t targetRowStride,
      const std::byte* pSource,
      size_t sourceRowStride,
      size_t sourceWidth,
      size_t sourceHeight,
      size_t bytesPerPixel);

  /**
   * @brief Copies pixels from a source image to a target image.
   *
   * If the source and target dimensions are the same, the source pixels are
   * copied exactly into the target. If not, the source image is scaled to fit
   * the target rectangle.
   *
   * The filtering algorithm for scaling is not specified, but can be assumed
   * to provide reasonably good quality.
   *
   * The source and target images must have the same number of channels and same
   * bytes per channel. If scaling is required, they must also use exactly 1
   * byte per channel. If any of these requirements are violated, this function
   * will return false and will not change any target pixels.
   *
   * The provided rectangles are validated to ensure that they fall within the
   * range of the images. If they do not, this function will return false and
   * will not change any pixels.
   *
   * @param target The image in which to write pixels.
   * @param targetPixels The pixels to write in the target.
   * @param source The image from which to read pixels.
   * @param sourcePixels The pixels to read from the target.
   * @returns True if the source image was blitted successfully into the target,
   * or false if the blit could not be completed due to invalid ranges or
   * incompatible formats.
   */
  static bool blitImage(
      CesiumGltf::ImageCesium& target,
      const PixelRectangle& targetPixels,
      const CesiumGltf::ImageCesium& source,
      const PixelRectangle& sourcePixels);
};

} // namespace CesiumGltfReader
