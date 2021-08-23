#include "CesiumGltf/ImageManipulation.h"
#include "CesiumGltf/ImageCesium.h"
#include <cassert>
#include <cstring>
#include <stb_image_resize.h>

using namespace CesiumGltf;

void ImageManipulation::unsafeBlitImage(
    std::byte* pTarget,
    size_t targetRowStride,
    const std::byte* pSource,
    size_t sourceRowStride,
    size_t sourceWidth,
    size_t sourceHeight,
    size_t bytesPerPixel) {
  size_t bytesToCopyPerRow = bytesPerPixel * sourceWidth;

  if (bytesToCopyPerRow == targetRowStride &&
      targetRowStride == sourceRowStride) {
    // Copy a single contiguous block with all data.
    std::memcpy(pTarget, pSource, sourceWidth * sourceHeight * bytesPerPixel);
  } else {
    // Copy row by row
    for (size_t j = 0; j < sourceHeight; ++j) {
      std::memcpy(pTarget, pSource, bytesToCopyPerRow);
      pTarget += targetRowStride;
      pSource += sourceRowStride;
    }
  }
}

// Copies a rectangle of a source image into another image.
void ImageManipulation::blitImage(
    ImageCesium& target,
    const PixelRectangle& targetPixels,
    const ImageCesium& source,
    const PixelRectangle& sourcePixels) {

  if (sourcePixels.x < 0 || sourcePixels.y < 0 || sourcePixels.width < 0 ||
      sourcePixels.height < 0 ||
      (sourcePixels.x + sourcePixels.width) > source.width ||
      (sourcePixels.y + sourcePixels.height) > source.height) {
    // Attempting to blit from outside the bounds of the source image.
    assert(false);
    return;
  }

  if (targetPixels.x < 0 || targetPixels.y < 0 ||
      (targetPixels.x + targetPixels.width) > target.width ||
      (targetPixels.y + targetPixels.height) > target.height) {
    // Attempting to blit outside the bounds of the target image.
    assert(false);
    return;
  }

  if (target.channels != source.channels ||
      target.bytesPerChannel != source.bytesPerChannel) {
    // Source and target image formats don't match; currently not supported.
    assert(false);
    return;
  }

  size_t bytesPerPixel = size_t(target.bytesPerChannel * target.channels);
  size_t bytesPerSourceRow = bytesPerPixel * size_t(source.width);
  size_t bytesPerTargetRow = bytesPerPixel * size_t(target.width);

  // Position both pointers at the start of the first row.
  std::byte* pTarget = target.pixelData.data();
  const std::byte* pSource = source.pixelData.data();
  pTarget += size_t(targetPixels.y) * bytesPerTargetRow +
             size_t(targetPixels.x) * bytesPerPixel;
  pSource += size_t(sourcePixels.y) * bytesPerSourceRow +
             size_t(sourcePixels.x) * bytesPerPixel;

  if (sourcePixels.width == targetPixels.width &&
      sourcePixels.height == targetPixels.height) {
    // Simple, unscaled, byte-for-byte image copy.
    unsafeBlitImage(
        pTarget,
        bytesPerTargetRow,
        pSource,
        bytesPerSourceRow,
        size_t(sourcePixels.width),
        size_t(sourcePixels.height),
        bytesPerPixel);
  } else {
    // Use STB to do the copy / scale
    stbir_resize_uint8(
        reinterpret_cast<const unsigned char*>(pSource),
        sourcePixels.width,
        sourcePixels.height,
        int(bytesPerSourceRow),
        reinterpret_cast<unsigned char*>(pTarget),
        targetPixels.width,
        targetPixels.height,
        int(bytesPerTargetRow),
        target.channels);
  }
}
