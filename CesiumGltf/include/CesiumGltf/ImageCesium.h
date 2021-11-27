#pragma once

#include "Library.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace CesiumGltf {

/**
 * @brief Supported compressed pixel formats.
 */
enum CESIUMGLTF_API CompressedPixelFormatCesium {
  KTX2,

  // Block compression formats.
  ETC1_RGB,
  ETC2_RGBA,
  BC1_RGB,
  BC3_RGBA,
  BC4_R,
  BC5_RG,
  BC7_RGBA,
  PVRTC1_4_RGB,
  PVRTC1_4_RGBA,
  ASTC_4x4_RGBA,
  PVRTC2_4_RGB,
  PVRTC2_4_RGBA,
  ETC2_EAC_R11,
  ETC2_EAC_RG11
};

/**
 * @brief Holds {@link Image} properties that are specific to the glTF loader
 * rather than part of the glTF spec.
 */
struct CESIUMGLTF_API ImageCesium final {
  /**
   * @brief The width of the image in pixels.
   */
  int32_t width = 0;

  /**
   * @brief The height of the image in pixels.
   */
  int32_t height = 0;

  /**
   * @brief The number of channels per pixel.
   */
  int32_t channels = 4;

  /**
   * @brief The number of bytes per channel.
   */
  int32_t bytesPerChannel = 1;

  /**
   * @brief The compressed pixel format, if this image is compressed.
   */
  std::optional<CompressedPixelFormatCesium> compressedPixelFormat =
      std::nullopt;

  /**
   * @brief The pixel data.
   *
   * This will be the raw pixel data when compressedPixelFormat is std::nullopt.
   * Otherwise, this buffer will store the compressed pixel data in the
   * specified format.
   *
   * When this is an uncompressed texture:
   * -The pixel data is consistent with the
   *  [stb](https://github.com/nothings/stb) image library.
   *
   * -For a correctly-formed image, the size of the array will be
   *  `width * height * channels * bytesPerChannel` bytes. There is no
   *  padding between rows or columns of the image, regardless of format.
   *
   * -The channels and their meaning are as follows:
   *
   * | Number of Channels | Channel Order and Meaning |
   * |--------------------|---------------------------|
   * | 1                  | grey                      |
   * | 2                  | grey, alpha               |
   * | 3                  | red, green, blue          |
   * | 4                  | red, green, blue, alpha   |
   */
  std::vector<std::byte> pixelData;
};
} // namespace CesiumGltf
