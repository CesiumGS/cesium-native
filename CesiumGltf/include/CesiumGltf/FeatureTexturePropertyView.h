#pragma once

#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/ExtensionModelExtFeatureMetadata.h"
#include "CesiumGltf/FeatureTexture.h"
#include "CesiumGltf/Sampler.h"
#include "CesiumGltf/Texture.h"
#include "CesiumGltf/TextureAccessor.h"
#include "CesiumGltf/TextureInfo.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"

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
  InvalidEmptyImage,
  InvalidChannelsString
};

enum class FeatureTexturePropertyComponentType {
  Uint8
  // TODO: add more types. Currently this is the only one outputted by stb,
  // so change stb call to output more of the original types.
};

struct FeatureTexturePropertyChannelOffsets {
  int32_t r = -1;
  int32_t g = -1;
  int32_t b = -1;
  int32_t a = -1;
};

template <typename T> struct FeatureTexturePropertyValue { T components[4]; };

class FeatureTexturePropertyView {
public:
  FeatureTexturePropertyView() noexcept;

  FeatureTexturePropertyView(
      const Model& model,
      const ClassProperty& classProperty,
      const TextureAccessor& textureAccessor) noexcept;

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
    property.components[0] = *(pRedChannel + this->_channelOffsets.r);
    property.components[1] = *(pRedChannel + this->_channelOffsets.g);
    property.components[2] = *(pRedChannel + this->_channelOffsets.b);
    property.components[3] = *(pRedChannel + this->_channelOffsets.a);

    return property;
  }

  constexpr FeatureTexturePropertyViewStatus status() const noexcept {
    return this->_status;
  }

  constexpr FeatureTexturePropertyComponentType
  getPropertyType() const noexcept {
    return this->_type;
  }

  constexpr int64_t getComponentCount() const noexcept {
    return this->_componentCount;
  }

  constexpr int64_t getTextureCoordinateIndex() const noexcept {
    return this->_textureCoordinateIndex;
  }

  constexpr bool isNormalized() const noexcept { return this->_normalized; }

  constexpr const ImageCesium* getImage() const noexcept {
    return this->_pImage;
  }

  constexpr const FeatureTexturePropertyChannelOffsets
  getChannelOffsets() const noexcept {
    return this->_channelOffsets;
  }

private:
  const Sampler* _pSampler;
  const ImageCesium* _pImage;
  const ClassProperty* _pClassProperty;
  int64_t _textureCoordinateIndex;
  FeatureTexturePropertyViewStatus _status;
  FeatureTexturePropertyChannelOffsets _channelOffsets;
  FeatureTexturePropertyComponentType _type;
  int64_t _componentCount;
  bool _normalized;
};
} // namespace CesiumGltf