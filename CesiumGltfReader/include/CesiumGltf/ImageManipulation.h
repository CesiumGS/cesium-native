#pragma once

#include "CesiumGltf/ReaderLibrary.h"
#include <cstddef>
#include <cstdint>

namespace CesiumGltf {

struct ImageCesium;

struct PixelRectangle {
  int32_t x;
  int32_t y;
  int32_t width;
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
   * If the source and target dimensions are not the same, the image is resized.
   * Otherwise, the source pixels are exactly copied into the target.
   *
   * The images must have the same number of channels and bytes per channel, and
   * the provided rectangles must be valid for the images. If any of these
   * constraints are violated, this function will assert in in debug builds and
   * silently do nothing in release builds.
   *
   * @param target The image in which to write pixels.
   * @param targetPixels The pixels to write in the target.
   * @param source The image from which to read pixels.
   * @param sourcePixels The pixels to read from the target.
   */
  static void blitImage(
      ImageCesium& target,
      const PixelRectangle& targetPixels,
      const ImageCesium& source,
      const PixelRectangle& sourcePixels);
};

} // namespace CesiumGltf
