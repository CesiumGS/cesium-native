#include "CesiumGltf/FeatureIdTextureView.h"

namespace CesiumGltf {
FeatureIdTextureView::FeatureIdTextureView() noexcept
    : _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _texCoordSetIndex(-1),
      _channels(),
      _pImage(nullptr) {}

FeatureIdTextureView::FeatureIdTextureView(
    const Model& model,
    const FeatureIdTexture& featureIdTexture) noexcept
    : _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _texCoordSetIndex(-1),
      _channels(featureIdTexture.channels),
      _pImage(nullptr) {
  int32_t textureIndex = featureIdTexture.index;
  if (textureIndex < 0 ||
      static_cast<size_t>(textureIndex) >= model.textures.size()) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidTexture;
    return;
  }

  const Texture& texture = model.textures[static_cast<size_t>(textureIndex)];
  if (texture.source < 0 ||
      static_cast<size_t>(texture.source) >= model.images.size()) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidImage;
    return;
  }

  // Ignore the texture's sampler, we will always use nearest pixel sampling.

  this->_pImage = &model.images[static_cast<size_t>(texture.source)].cesium;
  if (this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_status = FeatureIdTextureViewStatus::ErrorEmptyImage;
    return;
  }

  // TODO: once compressed texture support is merged, check that the image is
  // decompressed here.

  if (this->_pImage->bytesPerChannel > 1) {
    this->_status =
        FeatureIdTextureViewStatus::ErrorInvalidImageBytesPerChannel;
    return;
  }

  if (featureIdTexture.texCoord < 0) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidTexCoordSetIndex;
    return;
  }
  this->_texCoordSetIndex = featureIdTexture.texCoord;

  const std::vector<int64_t>& channels = featureIdTexture.channels;
  if (channels.size() == 0 || channels.size() > 4 ||
      channels.size() > static_cast<size_t>(this->_pImage->channels)) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidChannels;
    return;
  }

  // Only channel values 0-3 are supported.
  for (size_t i = 0; i < channels.size(); i++) {
    if (channels[i] < 0 || channels[i] > 3) {
      this->_status = FeatureIdTextureViewStatus::ErrorInvalidChannels;
      return;
    }
  }
  this->_channels = channels;

  this->_status = FeatureIdTextureViewStatus::Valid;
}

int64_t FeatureIdTextureView::getFeatureId(double u, double v) const noexcept {
  if (this->_status != FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  int64_t x = std::clamp(
      std::llround(u * this->_pImage->width),
      0LL,
      (long long)this->_pImage->width);
  int64_t y = std::clamp(
      std::llround(v * this->_pImage->height),
      0LL,
      (long long)this->_pImage->height);

  int64_t pixelOffset = this->_pImage->bytesPerChannel *
                        this->_pImage->channels *
                        (y * this->_pImage->width + x);

  int64_t value = 0;
  int64_t bitOffset = 0;
  // As stated in the spec: values from the selected channels are treated as
  // unsigned 8 bit integers, and represent the bytes of the actual feature ID,
  // in little-endian order.
  for (size_t i = 0; i < this->_channels.size(); i++) {
    int64_t channelValue = static_cast<int64_t>(
        this->_pImage
            ->pixelData[static_cast<size_t>(pixelOffset + this->_channels[i])]);
    value |= channelValue << bitOffset;
    bitOffset += 8;
  }

  return value;
}
} // namespace CesiumGltf
