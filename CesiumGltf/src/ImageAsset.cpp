#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/Assert.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace CesiumGltf {
void ImageAsset::writeTga(const std::string& outputPath) const {
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
  const uint16_t width16 = (uint16_t)this->width;
  stream.write(reinterpret_cast<const char*>(&width16), 2);
  // Height
  const uint16_t height16 = (uint16_t)this->height;
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
  for (int i = 0; i < this->width; i++) {
    for (int j = 0; j < this->height; j++) {
      // All images written as RGBA no matter the actual channel count
      // Blue
      stream.write(
          (this->channels > 2
               ? reinterpret_cast<const char*>(&this->pixelData[(
                     size_t)((i * this->width + j) * this->channels + 2)])
               : reinterpret_cast<const char*>(&zeroes)),
          1);
      // Green
      stream.write(
          (this->channels > 2
               ? reinterpret_cast<const char*>(&this->pixelData[(
                     size_t)((i * this->width + j) * this->channels + 1)])
               : reinterpret_cast<const char*>(&zeroes)),
          1);
      // Red
      stream.write(
          reinterpret_cast<const char*>(&this->pixelData[(
              size_t)((i * this->width + j) * this->channels)]),
          1);
      // Alpha
      if (this->channels == 2) {
        stream.write(
            reinterpret_cast<const char*>(&this->pixelData[(
                size_t)((i * this->width + j) * this->channels + 1)]),
            1);
      } else if (this->channels == 4) {
        stream.write(
            reinterpret_cast<const char*>(&this->pixelData[(
                size_t)((i * this->width + j) * this->channels + 3)]),
            1);
      } else {
        const uint8_t one = 1;
        stream.write(reinterpret_cast<const char*>(&one), 1);
      }
    }
  }
}

void ImageAsset::convertToChannels(
    int32_t newChannels,
    std::byte defaultValue) {
  if (newChannels == this->channels) {
    // Nothing to do.
    return;
  } else if (newChannels < this->channels) {
    // We're using fewer channels than previous, so we can perform the
    // conversion in-place.
    size_t writePos = 0;
    for (size_t i = 0; i < this->pixelData.size();
         i += (size_t)this->channels) {
      for (size_t j = 0; j < (size_t)newChannels; j++) {
        this->pixelData[writePos + j] = this->pixelData[i + j];
      }

      writePos += (size_t)newChannels;
    }

    this->pixelData.resize(writePos);
  } else {
    // We're using more channels than previous, so we need to make a new buffer.
    std::vector<std::byte> newPixelData(
        (size_t)(newChannels * this->width * this->height));
    size_t writePos = 0;
    for (size_t i = 0; i < this->pixelData.size();
         i += (size_t)this->channels) {
      for (size_t j = 0; j < (size_t)this->channels; j++) {
        newPixelData[writePos + j] = this->pixelData[i + j];
      }

      for (size_t j = 0; j < (size_t)(newChannels - this->channels); j++) {
        newPixelData[writePos + (size_t)this->channels + j] = defaultValue;
      }

      writePos += (size_t)newChannels;
    }

    this->pixelData = std::move(newPixelData);
  }
}
} // namespace CesiumGltf