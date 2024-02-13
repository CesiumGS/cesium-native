#include "CesiumGltf/FeatureIdTextureView.h"

#include "CesiumGltf/ExtensionKhrTextureTransform.h"
#include "CesiumGltf/SamplerUtility.h"

namespace CesiumGltf {
FeatureIdTextureView::FeatureIdTextureView() noexcept
    : _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _texCoordSetIndex(0),
      _channels(),
      _pImage(nullptr),
      _pSampler(nullptr),
      _textureTransform(std::nullopt) {}

FeatureIdTextureView::FeatureIdTextureView(
    const Model& model,
    const FeatureIdTexture& featureIdTexture) noexcept
    : _status(FeatureIdTextureViewStatus::ErrorUninitialized),
      _texCoordSetIndex(featureIdTexture.texCoord),
      _channels(),
      _pImage(nullptr),
      _pSampler(nullptr),
      _textureTransform(std::nullopt) {
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

  this->_pImage = &model.images[static_cast<size_t>(texture.source)].cesium;
  if (this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_status = FeatureIdTextureViewStatus::ErrorEmptyImage;
    return;
  }

  if (texture.sampler < 0 ||
      static_cast<size_t>(texture.sampler) >= model.samplers.size()) {
    this->_status = FeatureIdTextureViewStatus::ErrorInvalidSampler;
    return;
  }

  this->_pSampler = &model.samplers[static_cast<size_t>(texture.sampler)];

  // TODO: once compressed texture support is merged, check that the image is
  // decompressed here.

  if (this->_pImage->bytesPerChannel > 1) {
    this->_status =
        FeatureIdTextureViewStatus::ErrorInvalidImageBytesPerChannel;
    return;
  }

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

  const ExtensionKhrTextureTransform* pTextureTransform =
      featureIdTexture.getExtension<ExtensionKhrTextureTransform>();

  if (pTextureTransform) {
    this->_textureTransform = KhrTextureTransform(*pTextureTransform);

    if (pTextureTransform->texCoord) {
      // Override with the extension's texcoord.
      this->_texCoordSetIndex = *pTextureTransform->texCoord;
    }
  }
} // namespace CesiumGltf

int64_t FeatureIdTextureView::getFeatureID(double u, double v) const noexcept {
  if (this->_status != FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  if (this->_textureTransform &&
      this->_textureTransform->status() == KhrTextureTransformStatus::Valid) {
    glm::dvec2 transformedUv = this->_textureTransform->applyTransform(u, v);
    u = transformedUv.x;
    v = transformedUv.y;
  }

  u = applySamplerWrapS(u, this->_pSampler->wrapS);
  v = applySamplerWrapT(v, this->_pSampler->wrapT);

  // Always use nearest filtering, and use std::floor instead of std::round.
  // This is because filtering is supposed to consider the pixel centers. But
  // memory access here acts as sampling the beginning of the pixel. Example:
  // 0.4 * 2 = 0.8. In a 2x1 pixel image, that should be closer to the left
  // pixel's center. But it will round to 1.0 which corresponds to the right
  // pixel. So the right pixel has a bigger range than the left one, which is
  // incorrect.
  double xCoord = std::floor(u * this->_pImage->width);
  double yCoord = std::floor(v * this->_pImage->height);

  // Clamp to ensure no out-of-bounds data access
  int64_t x = glm::clamp(
      static_cast<int64_t>(xCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(this->_pImage->width - 1));
  int64_t y = glm::clamp(
      static_cast<int64_t>(yCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(this->_pImage->height - 1));

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
