#include "CesiumGltf/FeatureIDTextureView.h"

namespace CesiumGltf {

FeatureIDTextureView::FeatureIDTextureView() noexcept
    : _pImage(nullptr),
      _channel(0),
      _textureCoordinateAttributeId(-1),
      _featureTableName(),
      _status(FeatureIDTextureViewStatus::InvalidUninitialized) {}

FeatureIDTextureView::FeatureIDTextureView(
    const Model& model,
    const FeatureIDTexture& featureIDTexture) noexcept
    : _pImage(nullptr),
      _channel(0),
      _textureCoordinateAttributeId(-1),
      _featureTableName(featureIDTexture.featureTable),
      _status(FeatureIDTextureViewStatus::InvalidUninitialized) {

  this->_textureCoordinateAttributeId =
      featureIDTexture.featureIds.texture.texCoord;

  int32_t textureIndex = featureIDTexture.featureIds.texture.index;
  if (textureIndex < 0 ||
      static_cast<size_t>(textureIndex) >= model.textures.size()) {
    this->_status = FeatureIDTextureViewStatus::InvalidTextureIndex;
    return;
  }

  const Texture& texture = model.textures[static_cast<size_t>(textureIndex)];

  // Ignore sampler, we will always use nearest pixel sampling.
  if (texture.source < 0 ||
      static_cast<size_t>(texture.source) >= model.images.size()) {
    this->_status = FeatureIDTextureViewStatus::InvalidImageIndex;
    return;
  }

  this->_pImage = &model.images[static_cast<size_t>(texture.source)].cesium;

  // This assumes that if the channel is g, there must be at least two
  // channels (r and g). If it is b there must be r, g, and b. If there is a,
  // then there must be r, g, b, and a.
  if (featureIDTexture.featureIds.channels == "r") {
    this->_channel = 0;
  } else if (featureIDTexture.featureIds.channels == "g") {
    this->_channel = 1;
  } else if (featureIDTexture.featureIds.channels == "b") {
    this->_channel = 2;
  } else if (featureIDTexture.featureIds.channels == "a") {
    this->_channel = 3;
  } else {
    this->_status = FeatureIDTextureViewStatus::InvalidChannel;
    return;
  }

  if (this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_status = FeatureIDTextureViewStatus::InvalidEmptyImage;
    return;
  }

  // TODO: once compressed texture support is merged, check that the image is
  // decompressed here.

  this->_status = FeatureIDTextureViewStatus::Valid;
}

int64_t FeatureIDTextureView::getFeatureID(double u, double v) const noexcept {
  if (this->_status != FeatureIDTextureViewStatus::Valid) {
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

  return static_cast<int64_t>(
      this->_pImage
          ->pixelData[static_cast<size_t>(pixelOffset + this->_channel)]);
}
} // namespace CesiumGltf