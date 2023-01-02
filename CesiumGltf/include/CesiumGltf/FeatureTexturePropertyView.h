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

/**
 * @brief Indicates the status of a feature texture property view.
 *
 * The {@link FeatureTexturePropertyView} constructor always completes
 * successfully. However it may not always reflect the actual content of the
 * corresponding feature texture property. This enumeration provides the reason.
 */
enum class FeatureTexturePropertyViewStatus {
  /**
   * @brief This view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This view has not been initialized.
   */
  InvalidUninitialized,

  /**
   * @brief This feature texture property has a texture index that does not
   * exist in the glTF.
   */
  InvalidTextureIndex,

  /**
   * @brief This feature texture property has a texture sampler index that does
   * not exist in the glTF.
   */
  InvalidTextureSamplerIndex,

  /**
   * @brief This feature texture property has an image index that does not
   * exist in the glTF.
   */
  InvalidImageIndex,

  /**
   * @brief This feature texture property points to an empty image.
   */
  InvalidEmptyImage,

  /**
   * @brief This feature texture property has an invalid channels string.
   */
  InvalidChannelsString
};

/**
 * @brief The supported component types that can exist in feature id textures.
 */
enum class FeatureTexturePropertyComponentType {
  Uint8
  // TODO: add more types. Currently this is the only one outputted by stb,
  // so change stb call to output more of the original types.
};

/**
 * @brief Specifies which channel each component exists in or -1 if the channel
 * isn't present. This can be used to un-swizzle pixel data.
 */
struct FeatureTexturePropertyChannelOffsets {
  int32_t r = -1;
  int32_t g = -1;
  int32_t b = -1;
  int32_t a = -1;
};

/**
 * @brief The feature texture property value for a pixel. This will contain
 * four channels of the specified type.
 *
 * Only the first n components will be valid, where n is the number of channels
 * in this feature texture property.
 *
 * @tparam T The component type, must correspond to a valid
 * {@link FeatureTexturePropertyComponentType}.
 */
template <typename T> struct FeatureTexturePropertyValue { T components[4]; };

/**
 * @brief A view of the data specified by a property from a
 * {@link FeatureTexture}.
 *
 * Provides utilities to sample the feature texture property using texture
 * coordinates.
 */
class FeatureTexturePropertyView {
public:
  /**
   * @brief Construct an uninitialized, invalid view.
   */
  FeatureTexturePropertyView() noexcept;

  /**
   * @brief Construct a view of the data specified by a feature texture
   * property.
   *
   * @param model The glTF in which to look for the data specified by the
   * feature texture property.
   * @param classProperty The property description.
   * @param textureAccessor The texture accessor for this property.
   */
  FeatureTexturePropertyView(
      const Model& model,
      const ClassProperty& classProperty,
      const TextureAccessor& textureAccessor) noexcept;

  /**
   * @brief Get the property for the given texture coordinates.
   *
   * Will return -1s when the status is not Valid or when the templated
   * component type doesn't match the image's channel byte-size.
   *
   * @tparam T The component type to use when interpreting the channels of the
   * property's pixel value.
   * @param u The u-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @param v The v-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @return The property at the nearest pixel to the texture coordinates.
   */
  template <typename T>
  FeatureTexturePropertyValue<T>
  getProperty(double u, double v) const noexcept {
    if (this->_status != FeatureTexturePropertyViewStatus::Valid ||
        sizeof(T) != this->_pImage->bytesPerChannel) {
      FeatureTexturePropertyValue<T> property;
      property.components[0] = -1;
      property.components[1] = -1;
      property.components[2] = -1;
      property.components[3] = -1;
      return property;
    }

    // TODO: actually use the sampler??
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
    const T* pRedChannel = reinterpret_cast<const T*>(
        this->_pImage->pixelData.data() + pixelOffset);

    FeatureTexturePropertyValue<T> property;
    property.components[0] = *(pRedChannel + this->_channelOffsets.r);
    property.components[1] = *(pRedChannel + this->_channelOffsets.g);
    property.components[2] = *(pRedChannel + this->_channelOffsets.b);
    property.components[3] = *(pRedChannel + this->_channelOffsets.a);

    return property;
  }

  /**
   * @brief Get the status of this view.
   *
   * If invalid, it will not be safe to sample feature ids from this view.
   */
  FeatureTexturePropertyViewStatus status() const noexcept {
    return this->_status;
  }

  /**
   * @brief Get the component type for this property.
   */
  FeatureTexturePropertyComponentType getPropertyType() const noexcept {
    return this->_type;
  }

  /**
   * @brief Get the component count for this property.
   *
   * This is also how many channels a pixel value for this property will use.
   */
  int64_t getComponentCount() const noexcept { return this->_componentCount; }

  /**
   * @brief Get the texture coordinate attribute index for this property.
   */
  int64_t getTextureCoordinateAttributeId() const noexcept {
    return this->_textureCoordinateAttributeId;
  }

  /**
   * @brief Whether the component type for this property should be normalized.
   */
  bool isNormalized() const noexcept { return this->_normalized; }

  /**
   * @brief Get the image containing this property's data.
   *
   * This will be nullptr if the feature texture property view runs into
   * problems during construction.
   */
  const ImageCesium* getImage() const noexcept { return this->_pImage; }

  /**
   * @brief Get the swizzle string for this texture's channels. Used to
   * determine which channel represents red, green, blue, and alpha
   * respectively.
   */
  const std::string& getSwizzle() const noexcept {
    const static std::string empty_str = "";
    return this->_pSwizzle ? *this->_pSwizzle : empty_str;
  }

  /**
   * @brief Get the {@link FeatureTexturePropertyChannelOffsets} that specifies
   * how to un-swizzle this property's pixel values.
   */
  const FeatureTexturePropertyChannelOffsets&
  getChannelOffsets() const noexcept {
    return this->_channelOffsets;
  }

private:
  const Sampler* _pSampler;
  const ImageCesium* _pImage;
  const ClassProperty* _pClassProperty;
  const std::string* _pSwizzle;
  int64_t _textureCoordinateAttributeId;
  FeatureTexturePropertyViewStatus _status;
  FeatureTexturePropertyChannelOffsets _channelOffsets;
  FeatureTexturePropertyComponentType _type;
  int64_t _componentCount;
  bool _normalized;
};
} // namespace CesiumGltf