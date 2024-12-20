#include <CesiumGltf/FeatureIdTexture.h>
#include <CesiumGltf/FeatureIdTextureView.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/TextureView.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace CesiumGltf {
FeatureIdTextureView::FeatureIdTextureView() noexcept
    : TextureView(),
      _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _channels() {}

FeatureIdTextureView::FeatureIdTextureView(
    const Model& model,
    const FeatureIdTexture& featureIdTexture,
    const TextureViewOptions& options) noexcept
    : TextureView(model, featureIdTexture, options),
      _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _channels() {
  switch (this->getTextureViewStatus()) {
  case TextureViewStatus::Valid:
    break;
  case TextureViewStatus::ErrorInvalidSampler:
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidSampler;
    return;
  case TextureViewStatus::ErrorInvalidImage:
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidImage;
    return;
  case TextureViewStatus::ErrorEmptyImage:
    this->_status = FeatureIdTextureViewStatus::ErrorEmptyImage;
    return;
  case TextureViewStatus::ErrorInvalidBytesPerChannel:
    this->_status =
        FeatureIdTextureViewStatus::ErrorInvalidImageBytesPerChannel;
    return;
  case TextureViewStatus::ErrorUninitialized:
  case TextureViewStatus::ErrorInvalidTexture:
  default:
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidTexture;
    return;
  }

  const std::vector<int64_t>& channels = featureIdTexture.channels;
  if (channels.size() == 0 || channels.size() > 4 ||
      channels.size() > static_cast<size_t>(this->getImage()->channels)) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidChannels;
    return;
  }

  // Only channel values 0-3 are supported.
  for (int64_t channel : channels) {
    if (channel < 0 || channel > 3) {
      this->_status = FeatureIdTextureViewStatus::ErrorInvalidChannels;
      return;
    }
  }

  this->_channels = channels;
  this->_status = FeatureIdTextureViewStatus::Valid;
} // namespace CesiumGltf

int64_t FeatureIdTextureView::getFeatureID(double u, double v) const noexcept {
  if (this->_status != FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  std::vector<uint8_t> sample = this->sampleNearestPixel(u, v, this->_channels);

  int64_t value = 0;
  int64_t bitOffset = 0;
  // As stated in the spec: values from the selected channels are treated as
  // unsigned 8 bit integers, and represent the bytes of the actual feature ID,
  // in little-endian order.
  for (size_t i = 0; i < this->_channels.size(); i++) {
    int64_t channelValue = static_cast<int64_t>(sample[i]);
    value |= channelValue << bitOffset;
    bitOffset += 8;
  }

  return value;
}
} // namespace CesiumGltf
