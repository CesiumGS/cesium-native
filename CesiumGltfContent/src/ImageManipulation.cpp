#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfContent/ImageManipulation.h>
#include <CesiumGltfReader/ImageDecoder.h>

#include <cstddef>
#include <cstring>
#include <vector>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace CesiumGltfReader;

namespace CesiumGltfContent {

void ImageManipulation::unsafeBlitImage(
    std::byte* pTarget,
    size_t targetRowStride,
    const std::byte* pSource,
    size_t sourceRowStride,
    size_t sourceWidth,
    size_t sourceHeight,
    size_t bytesPerPixel) {
  const size_t bytesToCopyPerRow = bytesPerPixel * sourceWidth;

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

bool ImageManipulation::blitImage(
    CesiumGltf::ImageAsset& target,
    const PixelRectangle& targetPixels,
    const CesiumGltf::ImageAsset& source,
    const PixelRectangle& sourcePixels) {

  if (sourcePixels.x < 0 || sourcePixels.y < 0 || sourcePixels.width < 0 ||
      sourcePixels.height < 0 ||
      (sourcePixels.x + sourcePixels.width) > source.width ||
      (sourcePixels.y + sourcePixels.height) > source.height) {
    // Attempting to blit from outside the bounds of the source image.
    return false;
  }

  if (targetPixels.x < 0 || targetPixels.y < 0 ||
      (targetPixels.x + targetPixels.width) > target.width ||
      (targetPixels.y + targetPixels.height) > target.height) {
    // Attempting to blit outside the bounds of the target image.
    return false;
  }

  if (target.channels != source.channels ||
      target.bytesPerChannel != source.bytesPerChannel) {
    // Source and target image formats don't match; currently not supported.
    return false;
  }

  size_t bytesPerPixel = size_t(target.bytesPerChannel * target.channels);
  const size_t bytesPerSourceRow = bytesPerPixel * size_t(source.width);
  size_t bytesPerTargetRow = bytesPerPixel * size_t(target.width);

  const size_t requiredTargetSize =
      size_t(targetPixels.height) * bytesPerTargetRow;
  const size_t requiredSourceSize =
      size_t(sourcePixels.height) * bytesPerSourceRow;
  if (target.pixelData.size() < requiredTargetSize ||
      source.pixelData.size() < requiredSourceSize) {
    return false;
  }

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
    if (target.bytesPerChannel != 1) {
      // We currently only support resizing images that use 1 byte per channel.
      return false;
    }

    // Use STB to do the copy / scale
    ImageDecoder::unsafeResize(
        pSource,
        sourcePixels.width,
        sourcePixels.height,
        int(bytesPerSourceRow),
        pTarget,
        targetPixels.width,
        targetPixels.height,
        int(bytesPerTargetRow),
        target.channels);
  }

  return true;
}

namespace {
void writePngToVector(void* context, void* data, int size) {
  std::vector<std::byte>* pVector =
      reinterpret_cast<std::vector<std::byte>*>(context);
  size_t previousSize = pVector->size();
  pVector->resize(previousSize + size_t(size));
  std::memcpy(pVector->data() + previousSize, data, size_t(size));
}
} // namespace

/*static*/ void ImageManipulation::savePng(
    const CesiumGltf::ImageAsset& image,
    std::vector<std::byte>& output) {
  if (image.bytesPerChannel != 1) {
    // Only 8-bit images can be written.
    return;
  }

  stbi_write_png_to_func(
      writePngToVector,
      &output,
      image.width,
      image.height,
      image.channels,
      image.pixelData.data(),
      0);
}

/*static*/ std::vector<std::byte>
ImageManipulation::savePng(const CesiumGltf::ImageAsset& image) {
  std::vector<std::byte> result;
  savePng(image, result);
  return result;
}

} // namespace CesiumGltfContent
