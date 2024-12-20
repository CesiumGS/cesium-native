#pragma once

#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfReader/Library.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace CesiumGltfReader {

/**
 * @brief The result of reading an image with {@link ImageDecoder::readImage}.
 */
struct CESIUMGLTFREADER_API ImageReaderResult {

  /**
   * @brief The {@link CesiumGltf::ImageAsset} that was read.
   *
   * This will be `std::nullopt` if the image could not be read.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> pImage;

  /**
   * @brief Error messages that occurred while trying to read the image.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warning messages that occurred while reading the image.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Contains methods for reading and manipulating images.
 */
class ImageDecoder {
public:
  /**
   * @brief Reads an image from a buffer.
   *
   * The [stb_image](https://github.com/nothings/stb) library is used to decode
   * images in `JPG`, `PNG`, `TGA`, `BMP`, `PSD`, `GIF`, `HDR`, or `PIC` format.
   *
   * @param data The buffer from which to read the image.
   * @param ktx2TranscodeTargets The compression format to transcode
   * KTX v2 textures into. If this is std::nullopt, KTX v2 textures will be
   * fully decompressed into raw pixels.
   * @return The result of reading the image.
   */
  static ImageReaderResult readImage(
      const std::span<const std::byte>& data,
      const CesiumGltf::Ktx2TranscodeTargets& ktx2TranscodeTargets);

  /**
   * @brief Generate mipmaps for this image.
   *
   * Does nothing if mipmaps already exist or the compressedPixelFormat is not
   * GpuCompressedPixelFormat::NONE.
   *
   * @param image The image to generate mipmaps for.   *
   * @return A string describing the error, if unable to generate mipmaps.
   */
  static std::optional<std::string>
  generateMipMaps(CesiumGltf::ImageAsset& image);

  /**
   * @brief Resize an image, without validating the provided pointers or ranges.
   *
   * @param pInputPixels The input image.
   * @param inputWidth The width of the input image, in pixels.
   * @param inputHeight The height of the input image, in pixels.
   * @param inputStrideBytes The stride of the input image, in bytes. Stride is
   * the number of bytes between successive rows.
   * @param pOutputPixels The buffer into which to write the output image.
   * @param outputWidth The width of the output image, in pixels.
   * @param outputHeight The height of the otuput image, in pixels.
   * @param outputStrideBytes The stride of the output image, in bytes. Stride
   * is the number of bytes between successive rows.
   * @param channels The number of channels in both the input and output images.
   * @return True if the resize succeeded, false if it failed.
   */
  static bool unsafeResize(
      const std::byte* pInputPixels,
      int32_t inputWidth,
      int32_t inputHeight,
      int32_t inputStrideBytes,
      std::byte* pOutputPixels,
      int32_t outputWidth,
      int32_t outputHeight,
      int32_t outputStrideBytes,
      int32_t channels);
};

} // namespace CesiumGltfReader
