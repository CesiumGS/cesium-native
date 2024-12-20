#pragma once

#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltf/Library.h>
#include <CesiumUtility/SharedAsset.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace CesiumGltf {

/**
 * @brief The byte range within a buffer where this mip exists.
 */
struct CESIUMGLTF_API ImageAssetMipPosition {
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
 * @brief A 2D image asset, including its pixel data. The image may have
 * mipmaps, and it may be encoded in a GPU compression format.
 */
struct CESIUMGLTF_API ImageAsset final
    : public CesiumUtility::SharedAsset<ImageAsset> {
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
  std::vector<ImageAssetMipPosition> mipPositions;

  /**
   * @brief The pixel data.
   *
   * This will be the raw pixel data when compressedPixelFormat is std::nullopt.
   * Otherwise, this buffer will store the compressed pixel data in the
   * specified format.
   *
   * If mipPositions is not empty, this buffer will contains multiple mips
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

  /**
   * @brief The effective size of this image, in bytes, for estimating resource
   * usage for caching purposes.
   *
   * When this value is less than zero (the default), the size of this image
   * should be assumed to equal the size of the {@link pixelData} array. When
   * it is greater than or equal to zero, the specified size should be used
   * instead. For example, the overridden size may account for:
   *   * The `pixelData` being cleared during the load process in order to save
   * memory.
   *   * The cost of any renderer resources (e.g., GPU textures) created for
   * this image.
   */
  int64_t sizeBytes = -1;

  /**
   * @brief Constructs an empty image asset.
   */
  ImageAsset() = default;

  /**
   * @brief Gets the size of this asset, in bytes.
   *
   * If {@link sizeBytes} is greater than or equal to zero, it is returned.
   * Otherwise, the size of the {@link pixelData} array is returned.
   */
  int64_t getSizeBytes() const {
    return this->sizeBytes >= 0 ? this->sizeBytes
                                : static_cast<int64_t>(this->pixelData.size());
  }
};
} // namespace CesiumGltf
