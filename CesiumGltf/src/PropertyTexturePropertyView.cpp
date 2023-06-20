
#include "CesiumGltf/PropertyTexturePropertyView.h"

#include <unordered_set>

namespace CesiumGltf {
namespace {
static std::unordered_set<std::string> supportedTypes{
    ClassProperty::Type::SCALAR,
    ClassProperty::Type::VEC2,
    ClassProperty::Type::VEC3,
    ClassProperty::Type::VEC4};

bool isValidClassProperty(const ClassProperty& classProperty) {
  if (supportedTypes.find(classProperty.type) == supportedTypes.end()) {
    return false;
  }

  // Non-arrays don't need further validation.
  if (!classProperty.array) {
    return true;
  }

  if (classProperty.type != ClassProperty::Type::SCALAR) {
    return false;
  }

  int64_t count = classProperty.count.value_or(0);
  return count > 0 && count <= 4;
}
} // namespace

PropertyTexturePropertyView::PropertyTexturePropertyView() noexcept
    : _status(PropertyTexturePropertyViewStatus::ErrorUninitialized),
      _pClassProperty(nullptr),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(0),
      _channels(),
      _swizzle(""),
      _componentType(PropertyTexturePropertyComponentType::Uint8),
      _componentCount(0),
      _normalized(false) {}

PropertyTexturePropertyView::PropertyTexturePropertyView(
    const Model& model,
    const ClassProperty& classProperty,
    const PropertyTextureProperty& propertyTextureProperty) noexcept
    : _status(PropertyTexturePropertyViewStatus::ErrorUninitialized),
      _pClassProperty(&classProperty),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(propertyTextureProperty.texCoord),
      _channels(),
      _swizzle(""),
      _componentType(PropertyTexturePropertyComponentType::Uint8),
      _componentCount(0),
      _normalized(false) {

  if (!isValidClassProperty(classProperty)) {
    this->_status =
        PropertyTexturePropertyViewStatus::ErrorInvalidClassProperty;
    return;
  }

  if (classProperty.array) {
    this->_componentCount = *classProperty.count;
  } else if (classProperty.type == ClassProperty::Type::SCALAR) {
    this->_componentCount = 1;
  } else if (classProperty.type == ClassProperty::Type::VEC2) {
    this->_componentCount = 2;
  } else if (classProperty.type == ClassProperty::Type::VEC3) {
    this->_componentCount = 3;
  } else if (classProperty.type == ClassProperty::Type::VEC4) {
    this->_componentCount = 4;
  }

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

  // TODO: support more component types
  // this->_componentType = ...

  this->_normalized = this->_pClassProperty->normalized;

  // TODO: channels can represent a multi-byte value, so the last check will
  // need to change.
  const std::vector<int64_t>& channels = propertyTextureProperty.channels;
  if (channels.size() == 0 || channels.size() > 4 ||
      channels.size() > static_cast<size_t>(this->_pImage->channels) ||
      channels.size() != static_cast<size_t>(this->_componentCount)) {
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
