#pragma once

#include "CesiumGltf/ImageAsset.h"
#include "CesiumGltfReader/Library.h"

#include <CesiumUtility/IntrusivePointer.h>

#include <gsl/span>

#include <optional>
#include <string>
#include <vector>

namespace CesiumGltfReader {

/**
 * @brief The result of reading an image with {@link ImageDecoder::readImage}.
 */
struct CESIUMGLTFREADER_API ImageReaderResult {

  /**
   * @brief The {@link ImageAsset} that was read.
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
   * @param ktx2TranscodeTargetFormat The compression format to transcode
   * KTX v2 textures into. If this is std::nullopt, KTX v2 textures will be
   * fully decompressed into raw pixels.
   * @return The result of reading the image.
   */
  static ImageReaderResult readImage(
      const gsl::span<const std::byte>& data,
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
};

} // namespace CesiumGltfReader
