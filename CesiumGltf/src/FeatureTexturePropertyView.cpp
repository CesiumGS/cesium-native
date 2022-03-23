
#include "CesiumGltf/FeatureTexturePropertyView.h"

namespace CesiumGltf {

FeatureTexturePropertyView::FeatureTexturePropertyView() noexcept
    : _pSampler(nullptr),
      _pImage(nullptr),
      _pClassProperty(nullptr),
      _pSwizzle(nullptr),
      _textureCoordinateAttributeId(-1),
      _status(FeatureTexturePropertyViewStatus::InvalidUninitialized),
      _channelOffsets(),
      _type(FeatureTexturePropertyComponentType::Uint8),
      _componentCount(0),
      _normalized(false) {}

FeatureTexturePropertyView::FeatureTexturePropertyView(
    const Model& model,
    const ClassProperty& classProperty,
    const TextureAccessor& textureAccessor) noexcept
    : _pSampler(nullptr),
      _pImage(nullptr),
      _pClassProperty(&classProperty),
      _pSwizzle(&textureAccessor.channels),
      _textureCoordinateAttributeId(textureAccessor.texture.texCoord),
      _status(FeatureTexturePropertyViewStatus::InvalidUninitialized),
      _channelOffsets(),
      _type(FeatureTexturePropertyComponentType::Uint8),
      _componentCount(0),
      _normalized(false) {

  if (textureAccessor.texture.index < 0 ||
      static_cast<size_t>(textureAccessor.texture.index) >=
          model.textures.size()) {
    this->_status = FeatureTexturePropertyViewStatus::InvalidTextureIndex;
    return;
  }

  const Texture& texture =
      model.textures[static_cast<size_t>(textureAccessor.texture.index)];
  if (texture.sampler < 0 ||
      static_cast<size_t>(texture.sampler) >= model.samplers.size()) {
    this->_status =
        FeatureTexturePropertyViewStatus::InvalidTextureSamplerIndex;
    return;
  }

  this->_pSampler = &model.samplers[static_cast<size_t>(texture.sampler)];

  if (texture.source < 0 ||
      static_cast<size_t>(texture.source) >= model.images.size()) {
    this->_status = FeatureTexturePropertyViewStatus::InvalidImageIndex;
    return;
  }

  this->_pImage = &model.images[static_cast<size_t>(texture.source)].cesium;

  if (this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_status = FeatureTexturePropertyViewStatus::InvalidEmptyImage;
    return;
  }

  // TODO: support more types
  // this->_type = ...
  this->_componentCount = this->_pClassProperty->componentCount
                              ? *this->_pClassProperty->componentCount
                              : 1;
  this->_normalized = this->_pClassProperty->normalized;
  if (textureAccessor.channels.length() > 4 ||
      textureAccessor.channels.length() >
          static_cast<size_t>(this->_pImage->channels) ||
      textureAccessor.channels.length() !=
          static_cast<size_t>(this->_componentCount)) {
    this->_status = FeatureTexturePropertyViewStatus::InvalidChannelsString;
    return;
  }

  for (size_t i = 0; i < textureAccessor.channels.length(); ++i) {
    switch (textureAccessor.channels[i]) {
    case 'r':
      this->_channelOffsets.r = 0;
      break;
    case 'g':
      this->_channelOffsets.g = 1;
      break;
    case 'b':
      this->_channelOffsets.b = 2;
      break;
    case 'a':
      this->_channelOffsets.a = 3;
      break;
    default:
      this->_status = FeatureTexturePropertyViewStatus::InvalidChannelsString;
      return;
    }
  }

  this->_status = FeatureTexturePropertyViewStatus::Valid;
}
} // namespace CesiumGltf