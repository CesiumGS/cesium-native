#include <CesiumGltf/ImageAsset.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGltf {

void ImageAsset::changeNumberOfChannels(
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
        this->pixelData[writePos] = this->pixelData[i + j];
        ++writePos;
      }
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
        newPixelData[writePos] = this->pixelData[i + j];
        ++writePos;
      }

      for (size_t j = 0; j < (size_t)(newChannels - this->channels); j++) {
        newPixelData[writePos] = defaultValue;
        ++writePos;
      }
    }

    this->pixelData = std::move(newPixelData);
  }

  this->channels = newChannels;
}
} // namespace CesiumGltf