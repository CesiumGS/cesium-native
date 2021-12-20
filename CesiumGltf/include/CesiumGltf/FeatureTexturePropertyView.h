#pragma once

#include "ClassProperty.h"
#include "ExtensionModelExtFeatureMetadata.h"
#include "FeatureTexture.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"
#include "Sampler.h"
#include "Texture.h"
#include "TextureAccessor.h"
#include "TextureInfo.h"

#include <cassert>
#include <cstdint>
#include <variant>

namespace CesiumGltf {

enum class FeatureTexturePropertyViewStatus {
  Valid,
  InvalidUninitialized,
  InvalidTextureIndex,
  InvalidTextureSamplerIndex,
  InvalidTextureSourceIndex,
  InvalidChannelsString
};

enum class FeatureTexturePropertyComponentType {
  Uint8
  // TODO: add more types. Currently this is the only one outputted by stb,
  // so change stb call to output more of the original types.
};

template <typename T> struct FeatureTexturePropertyValue { T components[4]; };

class FeatureTexturePropertyView {
public:
  FeatureTexturePropertyView() noexcept
      : _pSampler(nullptr),
        _pImage(nullptr),
        _pClassProperty(nullptr),
        _textureCoordinateIndex(-1),
        _status(FeatureTexturePropertyViewStatus::InvalidUninitialized),
        _channelOffsets{-1, -1, -1, -1},
        _type(FeatureTexturePropertyComponentType::Uint8),
        _componentCount(0),
        _normalized(false) {}

  FeatureTexturePropertyView(
      const Model& model,
      const ClassProperty& classProperty,
      const TextureAccessor& textureAccessor) noexcept
      : _pSampler(nullptr),
        _pImage(nullptr),
        _pClassProperty(&classProperty),
        _textureCoordinateIndex(textureAccessor.texture.texCoord),
        _status(FeatureTexturePropertyViewStatus::InvalidUninitialized),
        _channelOffsets{-1, -1, -1, -1},
        _type(FeatureTexturePropertyComponentType::Uint8),
        _componentCount(0),
        _normalized(false) {

    if (textureAccessor.texture.index < 0 ||
        textureAccessor.texture.index >= model.textures.size()) {
      this->_status = FeatureTexturePropertyViewStatus::InvalidTextureIndex;
      return;
    }

    const Texture& texture = model.textures[textureAccessor.texture.index];
    if (texture.sampler < 0 || texture.sampler >= model.samplers.size()) {
      this->_status =
          FeatureTexturePropertyViewStatus::InvalidTextureSamplerIndex;
      return;
    }

    this->_pSampler = &model.samplers[texture.sampler];

    if (texture.source < 0 || texture.source >= model.images.size()) {
      this->_status =
          FeatureTexturePropertyViewStatus::InvalidTextureSourceIndex;
      return;
    }

    this->_pImage = &model.images[texture.source].cesium;

    // TODO: support more types
    // this->_type = ...
    this->_componentCount = this->_pClassProperty->componentCount
                                ? *this->_pClassProperty->componentCount
                                : 1;
    this->_normalized = this->_pClassProperty->normalized;
    if (textureAccessor.channels.length() > 4 ||
        textureAccessor.channels.length() > this->_pImage->channels ||
        textureAccessor.channels.length() !=
            static_cast<size_t>(this->_componentCount)) {
      this->_status = FeatureTexturePropertyViewStatus::InvalidChannelsString;
      return;
    }

    for (int i = 0; i < textureAccessor.channels.length(); ++i) {
      // TODO: should this be case-sensitive?
      switch (textureAccessor.channels[i]) {
      case 'r':
        this->_channelOffsets[i] = 0;
        break;
      case 'g':
        this->_channelOffsets[i] = 1;
        break;
      case 'b':
        this->_channelOffsets[i] = 2;
        break;
      case 'a':
        this->_channelOffsets[i] = 3;
        break;
      default:
        this->_status = FeatureTexturePropertyViewStatus::InvalidChannelsString;
        return;
      }
    }

    this->_status = FeatureTexturePropertyViewStatus::Valid;
  }

  /**
   * @brief Get the property for the given texture coordinates.
   *
   * Not safe when the status is not Valid.
   *
   * @param u The u-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @param v The v-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @return The property at the nearest pixel to the texture coordinates.
   */
  template <typename T>
  FeatureTexturePropertyValue<T>
  getProperty(double u, double v) const noexcept {
    assert(this->_status == FeatureTexturePropertyViewStatus::Valid);
    assert(sizeof(T) == this->_pImage->bytesPerChannel);

    // TODO: actually use the sampler??
    int64_t x = std::clamp(
        std::llround(u * this->_pImage->width),
        0LL,
        (int64_t)this->_pImage->width);
    int64_t y = std::clamp(
        std::llround(v * this->_pImage->height),
        0LL,
        (int64_t)this->_pImage->height);

    int64_t pixelOffset = this->_pImage->bytesPerChannel *
                          this->_pImage->channels *
                          (y * this->_pImage->width + x);
    const T* pRedChannel = reinterpret_cast<const T*>(
        this->_pImage->pixelData.data() + pixelOffset);

    FeatureTexturePropertyValue<T> property;
    for (int i = 0; i < this->_componentCount; ++i) {
      property.components[i] = *(pRedChannel + this->_channelOffsets[i]);
    }

    return property;
  }

  FeatureTexturePropertyViewStatus status() const noexcept {
    return this->_status;
  }

  FeatureTexturePropertyComponentType getPropertyType() const noexcept {
    return this->_type;
  }

  int64_t getComponentCount() const noexcept { return this->_componentCount; }

  int64_t getTextureCoordinateIndex() const noexcept {
    return this->_textureCoordinateIndex;
  }

  bool isNormalized() const noexcept { return this->_normalized; };

private:
  const Sampler* _pSampler;
  const ImageCesium* _pImage;
  const ClassProperty* _pClassProperty;
  int64_t _textureCoordinateIndex;
  FeatureTexturePropertyViewStatus _status;
  int32_t _channelOffsets[4];
  FeatureTexturePropertyComponentType _type;
  int64_t _componentCount;
  bool _normalized;
};
} // namespace CesiumGltf