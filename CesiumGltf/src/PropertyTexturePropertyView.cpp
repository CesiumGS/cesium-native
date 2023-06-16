
#include "CesiumGltf/PropertyTexturePropertyView.h"

namespace CesiumGltf {
PropertyTexturePropertyView::PropertyTexturePropertyView() noexcept
    : _status(PropertyTexturePropertyViewStatus::ErrorUninitialized),
      _pClassProperty(nullptr),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(-1),
      _channels(),
      _swizzle(""),
      _type(PropertyTexturePropertyComponentType::Uint8),
      _count(0),
      _normalized(false) {}

PropertyTexturePropertyView::PropertyTexturePropertyView(
    const Model& model,
    const ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTextureProperty&
        propertyTextureProperty) noexcept
    : _status(PropertyTexturePropertyViewStatus::ErrorUninitialized),
      _pClassProperty(&classProperty),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(propertyTextureProperty.texCoord),
      _channels(),
      _swizzle(""),
      _type(PropertyTexturePropertyComponentType::Uint8),
      _count(0),
      _normalized(false) {
  const int64_t index = propertyTextureProperty.index;
  if (index < 0 || static_cast<size_t>(index) >= model.textures.size()) {
    this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
    return;
  }

  const Texture& texture = model.textures[static_cast<size_t>(index)];
  if (texture.sampler < 0 ||
      static_cast<size_t>(texture.sampler) >= model.samplers.size()) {
    this->_status =
        PropertyTexturePropertyViewStatus::ErrorInvalidTextureSampler;
    return;
  }

  this->_pSampler = &model.samplers[static_cast<size_t>(texture.sampler)];

  if (texture.source < 0 ||
      static_cast<size_t>(texture.source) >= model.images.size()) {
    this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidImage;
    return;
  }

  this->_pImage = &model.images[static_cast<size_t>(texture.source)].cesium;
  if (this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_status = PropertyTexturePropertyViewStatus::ErrorEmptyImage;
    return;
  }

  if (this->_texCoordSetIndex < 0) {
    this->_status =
        PropertyTexturePropertyViewStatus::ErrorInvalidTexCoordSetIndex;
    return;
  }

  // TODO: support more types
  // this->_type = ...

  this->_count =
      this->_pClassProperty->count ? *this->_pClassProperty->count : 1;
  this->_normalized = this->_pClassProperty->normalized;

  const std::vector<int64_t>& channels = propertyTextureProperty.channels;
  if (channels.size() == 0 || channels.size() > 4 ||
      channels.size() > static_cast<size_t>(this->_pImage->channels) ||
      channels.size() != static_cast<size_t>(this->_count)) {
    this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
    return;
  }

  for (size_t i = 0; i < channels.size(); ++i) {
    switch (channels[i]) {
    case 0:
      this->_swizzle += "r";
      break;
    case 1:
      this->_swizzle += "g";
      break;
    case 2:
      this->_swizzle += "b";
      break;
    case 3:
      this->_swizzle += "a";
      break;
    default:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
      return;
    }
  }

  this->_channels = channels;

  this->_status = PropertyTexturePropertyViewStatus::Valid;
}
} // namespace CesiumGltf
