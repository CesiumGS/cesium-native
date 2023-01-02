#pragma once

#include "CesiumGltf/Ktx2TranscodeTargets.h"
#include "CesiumGltf/Library.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace CesiumGltf {

/**
 * @brief The byte range within a buffer where this mip exists.
 */
struct CESIUMGLTF_API ImageCesiumMipPosition {
  /**
   * @brief The byte index where this mip begins.
   */
  size_t byteOffset;

  /**
   * @brief The size in bytes of this mip.
   */
  size_t byteSize;
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
   * @brief The gpu compressed pixel format for this image or NONE if it is not
   * compressed.
   */
  GpuCompressedPixelFormat compressedPixelFormat =
      GpuCompressedPixelFormat::NONE;

  /**
   * @brief The offset of each mip in the pixel data.
   *
   * A list of the positions of each mip's data within the overall pixel buffer.
   * The first element will be the full image, the second will be the second
   * biggest and etc. If this is empty, assume the entire buffer is a single
   * image, the mip map will need to be generated on the client in this case.
   */
  std::vector<ImageCesiumMipPosition> mipPositions;

  /**
   * @brief The pixel data.
   *
   * This will be the raw pixel data when compressedPixelFormat is std::nullopt.
   * Otherwise, this buffer will store the compressed pixel data in the
   * specified format.
   *
   * If mipOffsets is not empty, this buffer will contains multiple mips
   * back-to-back.
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
