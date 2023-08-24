#pragma once

#include "CesiumGltf/ImageCesium.h"
#include "CesiumGltf/PropertyTextureProperty.h"
#include "CesiumGltf/PropertyTransformations.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumGltf/PropertyView.h"
#include "CesiumGltf/Sampler.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property texture property view.
 *
 * The {@link PropertyTexturePropertyView} constructor always completes
 * successfully. However it may not always reflect the actual content of the
 * corresponding property texture property. This enumeration provides the
 * reason.
 */
class PropertyTexturePropertyViewStatus : public PropertyViewStatus {
public:
  /**
   * @brief This property view was initialized from an invalid
   * {@link PropertyTexture}.
   */
  static const int ErrorInvalidPropertyTexture = 12;

  /**
   * @brief This property view is associated with a {@link ClassProperty} of an
   * unsupported type.
   */
  static const int ErrorUnsupportedProperty = 13;

  /**
   * @brief This property view does not have a valid texture index.
   */
  static const int ErrorInvalidTexture = 14;

  /**
   * @brief This property view does not have a valid sampler index.
   */
  static const int ErrorInvalidSampler = 15;

  /**
   * @brief This property view does not have a valid image index.
   */
  static const int ErrorInvalidImage = 16;

  /**
   * @brief This property is viewing an empty image.
   */
  static const int ErrorEmptyImage = 17;

  /**
   * @brief This property uses an image with multi-byte channels. Only
   * single-byte channels are supported.
   */
  static const int ErrorInvalidBytesPerChannel = 18;

  /**
   * @brief The channels of this property texture property are invalid.
   * Channels must be in the range 0-N, where N is the number of available
   * channels in the image. There must be a minimum of one channel. Although
   * more than four channels can be defined for specialized texture
   * formats, this implementation only supports four channels max.
   */
  static const int ErrorInvalidChannels = 19;

  /**
   * @brief The channels of this property texture property do not provide
   * the exact number of bytes required by the property type. This may be
   * because an incorrect number of channels was provided, or because the
   * image itself has a different channel count / byte size than expected.
   */
  static const int ErrorChannelsAndTypeMismatch = 20;
};

namespace {
template <typename ElementType>
ElementType assembleScalarValue(const std::vector<uint8_t>& bytes) {
  if constexpr (std::is_same_v<ElementType, float>) {
    assert(
        bytes.size() == sizeof(float) &&
        "Not enough channel inputs to construct a float.");
    uint32_t resultAsUint = 0;
    for (size_t i = 0; i < bytes.size(); i++) {
      resultAsUint |= static_cast<uint32_t>(bytes[i]) << i * 8;
    }

    // Reinterpret the bits as a float.
    return *reinterpret_cast<float*>(&resultAsUint);
  }

  if constexpr (IsMetadataInteger<ElementType>::value) {
    using UintType = std::make_unsigned_t<ElementType>;
    UintType resultAsUint = 0;
    for (size_t i = 0; i < bytes.size(); i++) {
      resultAsUint |= static_cast<UintType>(bytes[i]) << i * 8;
    }

    // Reinterpret the bits with the correct signedness.
    return *reinterpret_cast<ElementType*>(&resultAsUint);
  }
}

double applySamplerWrapS(const double u, const int32_t wrapS) {
  if (wrapS == Sampler::WrapS::REPEAT) {
    double integral = 0;
    double fraction = std::modf(u, &integral);
    return fraction < 0 ? 1.0 - fraction : fraction;
  }

  if (wrapS == Sampler::WrapS::MIRRORED_REPEAT) {
    double integral = 0;
    double fraction = std::abs(std::modf(u, &integral));
    int64_t integer = static_cast<int64_t>(std::abs(integral));
    // If the integer part is odd, the direction is reversed.
    return integer % 2 == 1 ? 1.0 - fraction : fraction;
  }

  return std::clamp(u, 0.0, 1.0);
}

double applySamplerWrapT(const double v, const int32_t wrapT) {
  if (wrapT == Sampler::WrapT::REPEAT) {
    double integral = 0;
    double fraction = std::modf(v, &integral);
    return fraction < 0 ? 1.0 - fraction : fraction;
  }

  if (wrapT == Sampler::WrapT::MIRRORED_REPEAT) {
    double integral = 0;
    double fraction = std::abs(std::modf(v, &integral));
    int64_t integer = static_cast<int64_t>(std::abs(integral));
    // If the integer part is odd, the direction is reversed.
    return integer % 2 == 1 ? 1.0 - fraction : fraction;
  }

  return std::clamp(v, 0.0, 1.0);
}
} // namespace

/**
 * @brief A view of the data specified by a {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates.
 */
template <typename ElementType, bool Normalized = false>
class PropertyTexturePropertyView;

/**
 * @brief A view of the non-normalized data specified by a
 * {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates.
 */
template <typename ElementType>
class PropertyTexturePropertyView<ElementType, false>
    : public PropertyView<ElementType, false> {
public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTexturePropertyView() noexcept
      : PropertyView<ElementType, false>(),
        _pSampler(nullptr),
        _pImage(nullptr),
        _texCoordSetIndex(0),
        _channels(),
        _swizzle("") {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The code from {@link PropertyTexturePropertyViewStatus} indicating the error with the property.
   */
  PropertyTexturePropertyView(PropertyViewStatusType status) noexcept
      : PropertyView<ElementType, false>(),
        _pSampler(nullptr),
        _pImage(nullptr),
        _texCoordSetIndex(0),
        _channels(),
        _swizzle("") {
    assert(
        _status != PropertyTexturePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Construct a view of the data specified by a {@link PropertyTextureProperty}.
   *
   * @param property The {@link PropertyTextureProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param pSampler A pointer to the sampler used by the property.
   * @param pImage A pointer to the image used by the property.
   * @param texCoordSetIndex The value of {@link PropertyTextureProperty::texcoord}.
   * @param channels The value of {@link PropertyTextureProperty::channels}.
   * @param normalized Whether this property has a normalized integer type.
   */
  PropertyTexturePropertyView(
      const PropertyTextureProperty& property,
      const ClassProperty& classProperty,
      const Sampler& sampler,
      const ImageCesium& image,
      int64_t texCoordSetIndex,
      const std::vector<int64_t>& channels) noexcept
      : PropertyView<ElementType, false>(classProperty, property),
        _pSampler(&sampler),
        _pImage(&image),
        _texCoordSetIndex(texCoordSetIndex),
        _channels(channels),
        _swizzle("") {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      return;
    }

    for (size_t i = 0; i < _channels.size(); ++i) {
      switch (_channels[i]) {
      case 0:
        _swizzle += "r";
        break;
      case 1:
        _swizzle += "g";
        break;
      case 2:
        _swizzle += "b";
        break;
      case 3:
        _swizzle += "a";
        break;
      default:
        assert(false && "A valid channels vector must be passed to the view.");
      }
    }
  }

  /**
   * @brief Gets the raw value of the property for the given texture
   * coordinates with all value transforms applied. That is, if the property
   * specifies an offset and scale, they will be applied to the value before the
   * value is returned. The sampler's wrapping mode will be used when sampling
   * the texture.
   *
   * If this property has a specified "no data" value, and the retrieved element
   * is equal to that value, then this will return the property's specified
   * default value. If the property did not provide a default value, this
   * returns std::nullopt.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<ElementType> get(double u, double v) const noexcept {
    ElementType value = getRaw(u, v);

    if (this->noData() && value == *(this->noData())) {
      return this->defaultValue();
    }

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      value = transformValue(value, this->offset(), this->scale());
    }

    if constexpr (IsMetadataNumericArray<ElementType>::value) {
      value = transformArray(value, this->offset(), this->scale());
    }

    return value;
  }

  /**
   * @brief Gets the raw value of the property for the given texture
   * coordinates. The sampler's wrapping mode will be used when sampling the
   * texture.
   *
   * If this property has a specified "no data" value, the raw value will still
   * be returned, even if it equals the "no data" value.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value at the nearest pixel to the texture coordinates.
   */

  ElementType getRaw(double u, double v) const noexcept {
    assert(
        _status == PropertyTexturePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");

    double wrappedU = applySamplerWrapS(u, this->_pSampler->wrapS);
    double wrappedV = applySamplerWrapT(v, this->_pSampler->wrapT);

    // TODO: account for sampler's filter (can be nearest or linear)

    // For nearest filtering, std::floor is used instead of std::round.
    // This is because filtering is supposed to consider the pixel centers. But
    // memory access here acts as sampling the beginning of the pixel. Example:
    // 0.4 * 2 = 0.8. In a 2x1 pixel image, that should be closer to the left
    // pixel's center. But it will round to 1.0 which corresponds to the right
    // pixel. So the right pixel has a bigger range than the left one, which is
    // incorrect.
    double xCoord = std::floor(wrappedU * this->_pImage->width);
    double yCoord = std::floor(wrappedV * this->_pImage->height);

    // Clamp to ensure no out-of-bounds data access
    int64_t x = std::clamp(
        static_cast<int64_t>(xCoord),
        static_cast<int64_t>(0),
        static_cast<int64_t>(this->_pImage->width) - 1);
    int64_t y = std::clamp(
        static_cast<int64_t>(yCoord),
        static_cast<int64_t>(0),
        static_cast<int64_t>(this->_pImage->height) - 1);

    int64_t pixelIndex = this->_pImage->bytesPerChannel *
                         this->_pImage->channels *
                         (y * this->_pImage->width + x);

    // TODO: Currently stb only outputs uint8 pixel types. If that
    // changes this should account for additional pixel byte sizes.
    const uint8_t* pValue = reinterpret_cast<const uint8_t*>(
        this->_pImage->pixelData.data() + pixelIndex);

    std::vector<uint8_t> channelValues(this->_channels.size());
    for (size_t i = 0; i < this->_channels.size(); i++) {
      channelValues[i] = *(pValue + this->_channels[i]);
    }

    return assembleValueFromChannels(channelValues);
  }

  /**
   * @brief Get the texture coordinate set index for this property.
   */
  int64_t getTexCoordSetIndex() const noexcept {
    return this->_texCoordSetIndex;
  }

  /**
   * @brief Get the image containing this property's data.
   *
   * This will be nullptr if the property texture property view runs into
   * problems during construction.
   */
  const ImageCesium* getImage() const noexcept { return this->_pImage; }

  /**
   * @brief Gets the channels of this property texture property.
   */
  const std::vector<int64_t>& getChannels() const noexcept {
    return this->_channels;
  }

  /**
   * @brief Gets this property's channels as a swizzle string.
   */
  const std::string& getSwizzle() const noexcept { return this->_swizzle; }

private:
  ElementType
  assembleValueFromChannels(const std::vector<uint8_t>& bytes) const noexcept {
    assert(bytes.size() > 0 && "Channel input must have at least one value.");

    if constexpr (IsMetadataScalar<ElementType>::value) {
      return assembleScalarValue<ElementType>(bytes);
    }

    if constexpr (IsMetadataVecN<ElementType>::value) {
      return assembleVecNValue(bytes);
    }

    if constexpr (IsMetadataArray<ElementType>::value) {
      return assembleArrayValue<typename MetadataArrayType<ElementType>::type>(
          bytes);
    }
  }

  ElementType
  assembleVecNValue(const std::vector<uint8_t>& bytes) const noexcept {
    const glm::length_t N =
        getDimensionsFromPropertyType(TypeToPropertyType<ElementType>::value);
    switch (N) {
    case 2:
      return assembleVecNValueImpl<2, typename ElementType::value_type>(bytes);
    case 3:
      return assembleVecNValueImpl<3, typename ElementType::value_type>(bytes);
    case 4:
      return assembleVecNValueImpl<4, typename ElementType::value_type>(bytes);
    default:
      return ElementType();
    }
  }

  template <glm::length_t N, typename T>
  ElementType
  assembleVecNValueImpl(const std::vector<uint8_t>& bytes) const noexcept {
    ElementType result = ElementType();
    assert(
        sizeof(T) <= 2 &&
        "Components cannot be larger than two bytes in size.");

    if constexpr (std::is_same_v<T, int16_t>) {
      assert(N == 2 && "Only vec2s can contain two-byte integer components.");
      uint16_t x = static_cast<uint16_t>(bytes[0]) |
                   (static_cast<uint16_t>(bytes[1]) << 8);
      uint16_t y = static_cast<uint16_t>(bytes[2]) |
                   (static_cast<uint16_t>(bytes[3]) << 8);

      result[0] = *reinterpret_cast<int16_t*>(&x);
      result[1] = *reinterpret_cast<int16_t*>(&y);
    }

    if constexpr (std::is_same_v<T, uint16_t>) {
      assert(N == 2 && "Only vec2s can contain two-byte integer components.");
      result[0] = static_cast<uint16_t>(bytes[0]) |
                  (static_cast<uint16_t>(bytes[1]) << 8);
      result[1] = static_cast<uint16_t>(bytes[2]) |
                  (static_cast<uint16_t>(bytes[3]) << 8);
    }

    if constexpr (std::is_same_v<T, int8_t>) {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = *reinterpret_cast<const int8_t*>(&bytes[i]);
      }
    }

    if constexpr (std::is_same_v<T, uint8_t>) {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = bytes[i];
      }
    }

    return result;
  }

  template <typename T>
  PropertyArrayView<T>
  assembleArrayValue(const std::vector<uint8_t>& bytes) const noexcept {
    std::vector<T> result(bytes.size() / sizeof(T));

    if constexpr (sizeof(T) == 2) {
      for (int i = 0, b = 0; i < result.size(); i++, b += 2) {
        using UintType = std::make_unsigned_t<T>;
        UintType resultAsUint = static_cast<UintType>(bytes[b]) |
                                (static_cast<UintType>(bytes[b + 1]) << 8);
        result[i] = *reinterpret_cast<T*>(&resultAsUint);
      }
    } else {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = *reinterpret_cast<const T*>(&bytes[i]);
      }
    }

    return PropertyArrayView<T>(std::move(result));
  }

  const Sampler* _pSampler;
  const ImageCesium* _pImage;
  int64_t _texCoordSetIndex;
  std::vector<int64_t> _channels;
  std::string _swizzle;
};

/**
 * @brief A view of the normalized data specified by a
 * {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates.
 */
template <typename ElementType>
class PropertyTexturePropertyView<ElementType, true>
    : public PropertyView<ElementType, true> {
private:
  using NormalizedType = typename TypeToNormalizedType<ElementType>::type;

public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTexturePropertyView() noexcept
      : PropertyView<ElementType, true>(),
        _pSampler(nullptr),
        _pImage(nullptr),
        _texCoordSetIndex(0),
        _channels(),
        _swizzle("") {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The code from {@link PropertyTexturePropertyViewStatus} indicating the error with the property.
   */
  PropertyTexturePropertyView(PropertyViewStatusType status) noexcept
      : PropertyView<ElementType, true>(),
        _pSampler(nullptr),
        _pImage(nullptr),
        _texCoordSetIndex(0),
        _channels(),
        _swizzle("") {
    assert(
        _status != PropertyTexturePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Construct a view of the data specified by a {@link PropertyTextureProperty}.
   *
   * @param property The {@link PropertyTextureProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param pSampler A pointer to the sampler used by the property.
   * @param pImage A pointer to the image used by the property.
   * @param texCoordSetIndex The value of {@link PropertyTextureProperty::texcoord}.
   * @param channels The value of {@link PropertyTextureProperty::channels}.
   * @param normalized Whether this property has a normalized integer type.
   */
  PropertyTexturePropertyView(
      const PropertyTextureProperty& property,
      const ClassProperty& classProperty,
      const Sampler& sampler,
      const ImageCesium& image,
      int64_t texCoordSetIndex,
      const std::vector<int64_t>& channels) noexcept
      : PropertyView<ElementType, true>(classProperty, property),
        _pSampler(&sampler),
        _pImage(&image),
        _texCoordSetIndex(texCoordSetIndex),
        _channels(channels),
        _swizzle("") {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      return;
    }

    for (size_t i = 0; i < _channels.size(); ++i) {
      switch (_channels[i]) {
      case 0:
        _swizzle += "r";
        break;
      case 1:
        _swizzle += "g";
        break;
      case 2:
        _swizzle += "b";
        break;
      case 3:
        _swizzle += "a";
        break;
      default:
        assert(false && "A valid channels vector must be passed to the view.");
      }
    }
  }

  /**
   * @brief Gets the value of the property for the given texture coordinates
   * with all value transforms applied. That is, if the property specifies an
   * offset and scale, they will be applied to the value before the value is
   * returned. The sampler's wrapping mode will be used when sampling the
   * texture.
   *
   * If this property has a specified "no data" value, and the retrieved element
   * is equal to that value, then this will return the property's specified
   * default value. If the property did not provide a default value, this
   * returns std::nullopt.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<NormalizedType> get(double u, double v) const noexcept {
    ElementType value = getRaw(u, v);

    if (this->noData() && value == *(this->noData())) {
      return this->defaultValue();
    }

    if constexpr (IsMetadataScalar<ElementType>::value) {
      return transformValue<NormalizedType>(
          normalize<ElementType>(value),
          this->offset(),
          this->scale());
    }

    if constexpr (IsMetadataVecN<ElementType>::value) {
      constexpr glm::length_t N = ElementType::length();
      using T = typename ElementType::value_type;
      using NormalizedT = typename NormalizedType::value_type;
      return transformValue<glm::vec<N, NormalizedT>>(
          normalize<N, T>(value),
          this->offset(),
          this->scale());
    }

    if constexpr (IsMetadataArray<ElementType>::value) {
      using ArrayElementType = typename MetadataArrayType<ElementType>::type;
      if constexpr (IsMetadataScalar<ArrayElementType>::value) {
        return transformNormalizedArray<ArrayElementType>(
            value,
            this->offset(),
            this->scale());
      }

      if constexpr (IsMetadataVecN<ArrayElementType>::value) {
        constexpr glm::length_t N = ArrayElementType::length();
        using T = typename ArrayElementType::value_type;
        return transformNormalizedVecNArray<N, T>(
            value,
            this->offset(),
            this->scale());
      }
    }
  }

  /**
   * @brief Gets the raw value of the property for the given texture
   * coordinates. The sampler's wrapping mode will be used when sampling the
   * texture.
   *
   * If this property has a specified "no data" value, the raw value will still
   * be returned, even if it equals the "no data" value.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value at the nearest pixel to the texture coordinates.
   */

  ElementType getRaw(double u, double v) const noexcept {
    assert(
        _status == PropertyTexturePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");

    double wrappedU = applySamplerWrapS(u, this->_pSampler->wrapS);
    double wrappedV = applySamplerWrapT(v, this->_pSampler->wrapT);

    // TODO: account for sampler's filter (can be nearest or linear)

    // For nearest filtering, std::floor is used instead of std::round.
    // This is because filtering is supposed to consider the pixel centers. But
    // memory access here acts as sampling the beginning of the pixel. Example:
    // 0.4 * 2 = 0.8. In a 2x1 pixel image, that should be closer to the left
    // pixel's center. But it will round to 1.0 which corresponds to the right
    // pixel. So the right pixel has a bigger range than the left one, which is
    // incorrect.
    double xCoord = std::floor(wrappedU * this->_pImage->width);
    double yCoord = std::floor(wrappedV * this->_pImage->height);

    // Clamp to ensure no out-of-bounds data access
    int64_t x = std::clamp(
        static_cast<int64_t>(xCoord),
        static_cast<int64_t>(0),
        static_cast<int64_t>(this->_pImage->width) - 1);
    int64_t y = std::clamp(
        static_cast<int64_t>(yCoord),
        static_cast<int64_t>(0),
        static_cast<int64_t>(this->_pImage->height) - 1);

    int64_t pixelIndex = this->_pImage->bytesPerChannel *
                         this->_pImage->channels *
                         (y * this->_pImage->width + x);

    // TODO: Currently stb only outputs uint8 pixel types. If that
    // changes this should account for additional pixel byte sizes.
    const uint8_t* pValue = reinterpret_cast<const uint8_t*>(
        this->_pImage->pixelData.data() + pixelIndex);

    std::vector<uint8_t> channelValues(this->_channels.size());
    for (size_t i = 0; i < this->_channels.size(); i++) {
      channelValues[i] = *(pValue + this->_channels[i]);
    }

    return assembleValueFromChannels(channelValues);
  }

  /**
   * @brief Get the texture coordinate set index for this property.
   */
  int64_t getTexCoordSetIndex() const noexcept {
    return this->_texCoordSetIndex;
  }

  /**
   * @brief Get the image containing this property's data.
   *
   * This will be nullptr if the property texture property view runs into
   * problems during construction.
   */
  const ImageCesium* getImage() const noexcept { return this->_pImage; }

  /**
   * @brief Gets the channels of this property texture property.
   */
  const std::vector<int64_t>& getChannels() const noexcept {
    return this->_channels;
  }

  /**
   * @brief Gets this property's channels as a swizzle string.
   */
  const std::string& getSwizzle() const noexcept { return this->_swizzle; }

private:
  ElementType
  assembleValueFromChannels(const std::vector<uint8_t>& bytes) const noexcept {
    assert(bytes.size() > 0 && "Channel input must have at least one value.");

    if constexpr (IsMetadataScalar<ElementType>::value) {
      return assembleScalarValue<ElementType>(bytes);
    }

    if constexpr (IsMetadataVecN<ElementType>::value) {
      return assembleVecNValue(bytes);
    }

    if constexpr (IsMetadataArray<ElementType>::value) {
      return assembleArrayValue<typename MetadataArrayType<ElementType>::type>(
          bytes);
    }
  }

  ElementType
  assembleVecNValue(const std::vector<uint8_t>& bytes) const noexcept {
    const glm::length_t N =
        getDimensionsFromPropertyType(TypeToPropertyType<ElementType>::value);
    switch (N) {
    case 2:
      return assembleVecNValueImpl<2, typename ElementType::value_type>(bytes);
    case 3:
      return assembleVecNValueImpl<3, typename ElementType::value_type>(bytes);
    case 4:
      return assembleVecNValueImpl<4, typename ElementType::value_type>(bytes);
    default:
      return ElementType();
    }
  }

  template <glm::length_t N, typename T>
  ElementType
  assembleVecNValueImpl(const std::vector<uint8_t>& bytes) const noexcept {
    ElementType result = ElementType();
    assert(
        sizeof(T) <= 2 &&
        "Components cannot be larger than two bytes in size.");

    if constexpr (std::is_same_v<T, int16_t>) {
      assert(N == 2 && "Only vec2s can contain two-byte integer components.");
      uint16_t x = static_cast<uint16_t>(bytes[0]) |
                   (static_cast<uint16_t>(bytes[1]) << 8);
      uint16_t y = static_cast<uint16_t>(bytes[2]) |
                   (static_cast<uint16_t>(bytes[3]) << 8);

      result[0] = *reinterpret_cast<int16_t*>(&x);
      result[1] = *reinterpret_cast<int16_t*>(&y);
    }

    if constexpr (std::is_same_v<T, uint16_t>) {
      assert(N == 2 && "Only vec2s can contain two-byte integer components.");
      result[0] = static_cast<uint16_t>(bytes[0]) |
                  (static_cast<uint16_t>(bytes[1]) << 8);
      result[1] = static_cast<uint16_t>(bytes[2]) |
                  (static_cast<uint16_t>(bytes[3]) << 8);
    }

    if constexpr (std::is_same_v<T, int8_t>) {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = *reinterpret_cast<const int8_t*>(&bytes[i]);
      }
    }

    if constexpr (std::is_same_v<T, uint8_t>) {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = bytes[i];
      }
    }

    return result;
  }

  template <typename T>
  PropertyArrayView<T>
  assembleArrayValue(const std::vector<uint8_t>& bytes) const noexcept {
    std::vector<T> result(bytes.size() / sizeof(T));

    if constexpr (sizeof(T) == 2) {
      for (int i = 0, b = 0; i < result.size(); i++, b += 2) {
        using UintType = std::make_unsigned_t<T>;
        UintType resultAsUint = static_cast<UintType>(bytes[b]) |
                                (static_cast<UintType>(bytes[b + 1]) << 8);
        result[i] = *reinterpret_cast<T*>(&resultAsUint);
      }
    } else {
      for (size_t i = 0; i < bytes.size(); i++) {
        result[i] = *reinterpret_cast<const T*>(&bytes[i]);
      }
    }

    return PropertyArrayView<T>(std::move(result));
  }

  const Sampler* _pSampler;
  const ImageCesium* _pImage;
  int64_t _texCoordSetIndex;
  std::vector<int64_t> _channels;
  std::string _swizzle;
};

} // namespace CesiumGltf
