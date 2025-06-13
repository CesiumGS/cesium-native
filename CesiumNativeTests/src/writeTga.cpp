#include <CesiumGltf/ImageAsset.h>
#include <CesiumNativeTests/writeTga.h>
#include <CesiumUtility/Assert.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace CesiumNativeTests {
namespace {
void writeTgaImpl(
    const std::filesystem::path& outputPath,
    const std::byte* pData,
    int32_t channels,
    int32_t width,
    int32_t height) {
  std::ofstream stream(outputPath, std::ios::binary | std::ios::out);
  CESIUM_ASSERT(stream.good());

  uint64_t zeroes = 0;

  // No image identification field included.
  stream.write(reinterpret_cast<const char*>(&zeroes), 1);
  // No color map included - raw RGB.
  stream.write(reinterpret_cast<const char*>(&zeroes), 1);
  // Data Type 2 - Unmapped RGB
  const uint8_t dataType = 2;
  stream.write(reinterpret_cast<const char*>(&dataType), 1);
  // Color Map Specification is five bytes long - we don't need it, but we need
  // to write that many bytes.
  stream.write(reinterpret_cast<const char*>(&zeroes), 5);
  // X origin
  stream.write(reinterpret_cast<const char*>(&zeroes), 2);
  // Y origin
  stream.write(reinterpret_cast<const char*>(&zeroes), 2);
  // Width
  const uint16_t width16 = (uint16_t)width;
  stream.write(reinterpret_cast<const char*>(&width16), 2);
  // Height
  const uint16_t height16 = (uint16_t)height;
  stream.write(reinterpret_cast<const char*>(&height16), 2);
  // Bits per pixel
  const uint8_t bpp = 32;
  stream.write(reinterpret_cast<const char*>(&bpp), 1);
  // Image Descriptor Byte
  // Bits 0-3 are the attribute bit count. For Targa 32 it's 8 bits. We don't
  // care about the other flags.
  const uint8_t descriptor = 8;
  stream.write(reinterpret_cast<const char*>(&descriptor), 1);
  // No image identification field
  // No color map data

  // Image Data Field
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      // All images written as RGBA no matter the actual channel count
      // Blue
      stream.write(
          (channels > 2 ? reinterpret_cast<const char*>(
                              &pData[(size_t)((i * width + j) * channels + 2)])
                        : reinterpret_cast<const char*>(&zeroes)),
          1);
      // Green
      stream.write(
          (channels > 2 ? reinterpret_cast<const char*>(
                              &pData[(size_t)((i * width + j) * channels + 1)])
                        : reinterpret_cast<const char*>(&zeroes)),
          1);
      // Red
      stream.write(
          reinterpret_cast<const char*>(
              &pData[(size_t)((i * width + j) * channels)]),
          1);
      // Alpha
      if (channels == 2) {
        stream.write(
            reinterpret_cast<const char*>(
                &pData[(size_t)((i * width + j) * channels + 1)]),
            1);
      } else if (channels == 4) {
        stream.write(
            reinterpret_cast<const char*>(
                &pData[(size_t)((i * width + j) * channels + 3)]),
            1);
      } else {
        const uint8_t one = 1;
        stream.write(reinterpret_cast<const char*>(&one), 1);
      }
    }
  }
}
} // namespace

void writeImageToTgaFile(
    const CesiumGltf::ImageAsset& image,
    const std::string& outputPath) {
  if (image.mipPositions.size() == 0) {
    writeTgaImpl(
        outputPath,
        image.pixelData.data(),
        image.channels,
        image.width,
        image.height);
  } else {
    std::filesystem::path outputPathParsed(outputPath);
    for (size_t i = 0; i < image.mipPositions.size(); i++) {
      std::filesystem::path thisPath(fmt::format(
          "{}-mip{}{}",
          outputPathParsed.stem().string(),
          i,
          outputPathParsed.extension().string()));
      writeTgaImpl(
          thisPath,
          image.pixelData.data() + image.mipPositions[i].byteOffset,
          image.channels,
          image.width >> i,
          image.height >> i);
    }
  }
}
} // namespace CesiumNativeTests