#pragma once

#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumGltf/PropertyView.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/TextureView.h>
#include <CesiumUtility/Assert.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

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
  static const int ErrorInvalidPropertyTexture = 14;

  /**
   * @brief This property view is associated with a {@link ClassProperty} of an
   * unsupported type.
   */
  static const int ErrorUnsupportedProperty = 15;

  /**
   * @brief This property view does not have a valid texture index.
   */
  static const int ErrorInvalidTexture = 16;

  /**
   * @brief This property view does not have a valid sampler index.
   */
  static const int ErrorInvalidSampler = 17;

  /**
   * @brief This property view does not have a valid image index.
   */
  static const int ErrorInvalidImage = 18;

  /**
   * @brief This property is viewing an empty image.
   */
  static const int ErrorEmptyImage = 19;

  /**
   * @brief This property uses an image with multi-byte channels. Only
   * single-byte channels are supported.
   */
  static const int ErrorInvalidBytesPerChannel = 20;

  /**
   * @brief The channels of this property texture property are invalid.
   * Channels must be in the range 0-N, where N is the number of available
   * channels in the image. There must be a minimum of one channel. Although
   * more than four channels can be defined for specialized texture
   * formats, this implementation only supports four channels max.
   */
  static const int ErrorInvalidChannels = 21;

  /**
   * @brief The channels of this property texture property do not provide
   * the exact number of bytes required by the property type. This may be
   * because an incorrect number of channels was provided, or because the
   * image itself has a different channel count / byte size than expected.
   */
  static const int ErrorChannelsAndTypeMismatch = 22;
};

/**
 * @brief Attempts to obtain a scalar value from the given span of bytes.
 *
 * @tparam ElementType The scalar value type to read from `bytes`.
 * @param bytes A span of bytes to convert into a scalar value.
 * @returns A value of `ElementType`.
 */
template <typename ElementType>
ElementType assembleScalarValue(const std::span<uint8_t> bytes) noexcept {
  if constexpr (std::is_same_v<ElementType, float>) {
    CESIUM_ASSERT(
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

/**
 * @brief Attempts to obtain a vector value from the given span of bytes.
 *
 * @tparam ElementType The vector value type to read from `bytes`.
 * @param bytes A span of bytes to convert into a vector value.
 * @returns A value of `ElementType`.
 */
template <typename ElementType>
ElementType assembleVecNValue(const std::span<uint8_t> bytes) noexcept {
  ElementType result = ElementType();

  const glm::length_t N =
      getDimensionsFromPropertyType(TypeToPropertyType<ElementType>::value);
  using T = typename ElementType::value_type;

  CESIUM_ASSERT(
      sizeof(T) <= 2 && "Components cannot be larger than two bytes in size.");

  if constexpr (std::is_same_v<T, int16_t>) {
    CESIUM_ASSERT(
        N == 2 && "Only vec2s can contain two-byte integer components.");
    uint16_t x = static_cast<uint16_t>(bytes[0]) |
                 (static_cast<uint16_t>(bytes[1]) << 8);
    uint16_t y = static_cast<uint16_t>(bytes[2]) |
                 (static_cast<uint16_t>(bytes[3]) << 8);

    result[0] = *reinterpret_cast<int16_t*>(&x);
    result[1] = *reinterpret_cast<int16_t*>(&y);
  }

  if constexpr (std::is_same_v<T, uint16_t>) {
    CESIUM_ASSERT(
        N == 2 && "Only vec2s can contain two-byte integer components.");
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

/**
 * @brief Attempts to obtain an array value from the given span of bytes.
 *
 * @tparam T The element type to read from `bytes`.
 * @param bytes A span of bytes to convert into an array value.
 * @returns A \ref PropertyArrayCopy containing the elements read.
 */
template <typename T>
PropertyArrayCopy<T>
assembleArrayValue(const std::span<uint8_t> bytes) noexcept {
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

  return PropertyArrayCopy<T>(std::move(result));
}

/**
 * @brief Assembles the given type from the provided channel values of sampling
 * a texture.
 *
 * @tparam ElementType The type of element to assemble.
 * @param bytes The byte values of the sampled channels of the texture.
 * @returns The result of \ref assembleScalarValue, \ref assembleVecNValue, or
 * \ref assembleArrayValue depending on `ElementType`.
 */
template <typename ElementType>
PropertyValueViewToCopy<ElementType>
assembleValueFromChannels(const std::span<uint8_t> bytes) noexcept {
  CESIUM_ASSERT(
      bytes.size() > 0 && "Channel input must have at least one value.");

  if constexpr (IsMetadataScalar<ElementType>::value) {
    return assembleScalarValue<ElementType>(bytes);
  }

  if constexpr (IsMetadataVecN<ElementType>::value) {
    return assembleVecNValue<ElementType>(bytes);
  }

  if constexpr (IsMetadataArray<ElementType>::value) {
    return assembleArrayValue<typename MetadataArrayType<ElementType>::type>(
        bytes);
  }
}

#pragma region Non - normalized property

/**
 * @brief A view of the data specified by a {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates. Property values are retrieved from the NEAREST texel without
 * additional filtering applied.
 *
 * @tparam ElementType The type of the elements represented in the property
 * view
 * @tparam Normalized Whether or not the property is normalized. If
 * normalized, the elements can be retrieved as normalized floating-point
 * numbers, as opposed to their integer values.
 */
template <typename ElementType, bool Normalized = false>
class PropertyTexturePropertyView;

/**
 * @brief A view of the non-normalized data specified by a
 * {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates.
 *
 * @tparam ElementType The type of the elements represented in the property view
 */
template <typename ElementType>
class PropertyTexturePropertyView<ElementType, false>
    : public PropertyView<ElementType, false>, public TextureView {
public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTexturePropertyView() noexcept
      : PropertyView<ElementType, false>(),
        TextureView(),
        _channels(),
        _swizzle() {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The code from {@link PropertyTexturePropertyViewStatus} indicating the error with the property.
   */
  PropertyTexturePropertyView(PropertyViewStatusType status) noexcept
      : PropertyView<ElementType, false>(status),
        TextureView(),
        _channels(),
        _swizzle() {
    CESIUM_ASSERT(
        this->_status != PropertyTexturePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid status");
  }

  /**
   * @brief Constructs an instance of an empty property that specifies a default
   * value. Although this property has no data, it can return the default value
   * when \ref get is called. However, \ref getRaw cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   */
  PropertyTexturePropertyView(const ClassProperty& classProperty) noexcept
      : PropertyView<ElementType, false>(classProperty),
        TextureView(),
        _channels(),
        _swizzle() {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      // Don't override the status / size if something is wrong with the class
      // property's definition.
      return;
    }

    if (!classProperty.defaultProperty) {
      // This constructor should only be called if the class property *has* a
      // default value. But in the case that it does not, this property view
      // becomes invalid.
      this->_status =
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty;
      return;
    }

    this->_status = PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault;
  }

  /**
   * @brief Construct a view of the data specified by a {@link PropertyTextureProperty}.
   *
   * @param property The {@link PropertyTextureProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param sampler The {@link Sampler} used by the property.
   * @param image The {@link ImageAsset} used by the property.
   * @param options The options for constructing the view.
   */
  PropertyTexturePropertyView(
      const PropertyTextureProperty& property,
      const ClassProperty& classProperty,
      const Sampler& sampler,
      const ImageAsset& image,
      const TextureViewOptions& options = TextureViewOptions()) noexcept
      : PropertyView<ElementType, false>(classProperty, property),
        TextureView(
            sampler,
            image,
            property.texCoord,
            property.getExtension<ExtensionKhrTextureTransform>(),
            options),
        _channels(property.channels),
        _swizzle() {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      return;
    }

    switch (this->getTextureViewStatus()) {
    case TextureViewStatus::Valid:
      break;
    case TextureViewStatus::ErrorInvalidSampler:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidSampler;
      return;
    case TextureViewStatus::ErrorInvalidImage:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidImage;
      return;
    case TextureViewStatus::ErrorEmptyImage:
      this->_status = PropertyTexturePropertyViewStatus::ErrorEmptyImage;
      return;
    case TextureViewStatus::ErrorInvalidBytesPerChannel:
      this->_status =
          PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel;
      return;
    case TextureViewStatus::ErrorUninitialized:
    case TextureViewStatus::ErrorInvalidTexture:
    default:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
      return;
    }

    _swizzle.reserve(_channels.size());

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
        CESIUM_ASSERT(
            false && "A valid channels vector must be passed to the view.");
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
   * If this property has a specified "no data" value, this will return the
   * property's default value for any elements that equal this "no data" value.
   * If the property did not specify a default value, this returns std::nullopt.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<PropertyValueViewToCopy<ElementType>>
  get(double u, double v) const noexcept {
    if (this->_status ==
        PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault) {
      return propertyValueViewToCopy(this->defaultValue());
    }

    PropertyValueViewToCopy<ElementType> value = getRaw(u, v);

    if (value == this->noData()) {
      return propertyValueViewToCopy(this->defaultValue());
    } else if constexpr (IsMetadataNumeric<ElementType>::value) {
      return transformValue(value, this->offset(), this->scale());
    } else if constexpr (IsMetadataNumericArray<ElementType>::value) {
      return transformArray(
          propertyValueCopyToView(value),
          this->offset(),
          this->scale());
    } else {
      return value;
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

  PropertyValueViewToCopy<ElementType>
  getRaw(double u, double v) const noexcept {
    CESIUM_ASSERT(
        this->_status == PropertyTexturePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");

    std::vector<uint8_t> sample =
        this->sampleNearestPixel(u, v, this->_channels);

    return assembleValueFromChannels<ElementType>(
        std::span(sample.data(), this->_channels.size()));
  }

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
  std::vector<int64_t> _channels;
  std::string _swizzle;
};

#pragma endregion

#pragma region Normalized property

/**
 * @brief A view of the normalized data specified by a
 * {@link PropertyTextureProperty}.
 *
 * Provides utilities to sample the property texture property using texture
 * coordinates.
 */
template <typename ElementType>
class PropertyTexturePropertyView<ElementType, true>
    : public PropertyView<ElementType, true>, public TextureView {
private:
  using NormalizedType = typename TypeToNormalizedType<ElementType>::type;

public:
  /**
   * @brief Constructs an invalid instance for a non-existent property.
   */
  PropertyTexturePropertyView() noexcept
      : PropertyView<ElementType, true>(),
        TextureView(),
        _channels(),
        _swizzle() {}

  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The code from {@link PropertyTexturePropertyViewStatus} indicating the error with the property.
   */
  PropertyTexturePropertyView(PropertyViewStatusType status) noexcept
      : PropertyView<ElementType, true>(status),
        TextureView(),
        _channels(),
        _swizzle() {
    CESIUM_ASSERT(
        this->_status != PropertyTexturePropertyViewStatus::Valid &&
        "An empty property view should not be constructed with a valid "
        "status");
  }

  /**
   * @brief Constructs an instance of an empty property that specifies a
   * default value. Although this property has no data, it can return the
   * default value when {@link PropertyTexturePropertyView<ElementType, true>::get} is called.
   * However, {@link PropertyTexturePropertyView<ElementType, true>::getRaw} cannot be used.
   *
   * @param classProperty The {@link ClassProperty} this property conforms to.
   */
  PropertyTexturePropertyView(const ClassProperty& classProperty) noexcept
      : PropertyView<ElementType, true>(classProperty),
        TextureView(),
        _channels(),
        _swizzle() {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      // Don't override the status / size if something is wrong with the class
      // property's definition.
      return;
    }

    if (!classProperty.defaultProperty) {
      // This constructor should only be called if the class property *has* a
      // default value. But in the case that it does not, this property view
      // becomes invalid.
      this->_status =
          PropertyTexturePropertyViewStatus::ErrorNonexistentProperty;
      return;
    }

    this->_status = PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault;
  }

  /**
   * @brief Construct a view of the data specified by a {@link PropertyTextureProperty}.
   *
   * @param property The {@link PropertyTextureProperty}
   * @param classProperty The {@link ClassProperty} this property conforms to.
   * @param sampler The {@link Sampler} used by the property.
   * @param image The {@link ImageAsset} used by the property.
   * @param options The options for constructing the view.
   */
  PropertyTexturePropertyView(
      const PropertyTextureProperty& property,
      const ClassProperty& classProperty,
      const Sampler& sampler,
      const ImageAsset& image,
      const TextureViewOptions& options = TextureViewOptions()) noexcept
      : PropertyView<ElementType, true>(classProperty, property),
        TextureView(
            sampler,
            image,
            property.texCoord,
            property.getExtension<ExtensionKhrTextureTransform>(),
            options),
        _channels(property.channels),
        _swizzle() {
    if (this->_status != PropertyTexturePropertyViewStatus::Valid) {
      return;
    }

    switch (this->getTextureViewStatus()) {
    case TextureViewStatus::Valid:
      break;
    case TextureViewStatus::ErrorInvalidSampler:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidSampler;
      return;
    case TextureViewStatus::ErrorInvalidImage:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidImage;
      return;
    case TextureViewStatus::ErrorEmptyImage:
      this->_status = PropertyTexturePropertyViewStatus::ErrorEmptyImage;
      return;
    case TextureViewStatus::ErrorInvalidBytesPerChannel:
      this->_status =
          PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel;
      return;
    case TextureViewStatus::ErrorUninitialized:
    case TextureViewStatus::ErrorInvalidTexture:
    default:
      this->_status = PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
      return;
    }

    _swizzle.reserve(_channels.size());
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
        CESIUM_ASSERT(
            false && "A valid channels vector must be passed to the view.");
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
   * If this property has a specified "no data" value, and the retrieved
   * element is equal to that value, then this will return the property's
   * specified default value. If the property did not provide a default value,
   * this returns std::nullopt.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value of the element, or std::nullopt if it matches the "no
   * data" value
   */
  std::optional<PropertyValueViewToCopy<NormalizedType>>
  get(double u, double v) const noexcept {
    if (this->_status ==
        PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault) {
      return propertyValueViewToCopy(this->defaultValue());
    }

    PropertyValueViewToCopy<ElementType> value = getRaw(u, v);

    if (value == this->noData()) {
      return propertyValueViewToCopy(this->defaultValue());
    } else if constexpr (IsMetadataScalar<ElementType>::value) {
      return transformValue<NormalizedType>(
          normalize<ElementType>(value),
          this->offset(),
          this->scale());
    } else if constexpr (IsMetadataVecN<ElementType>::value) {
      constexpr glm::length_t N = ElementType::length();
      using T = typename ElementType::value_type;
      using NormalizedT = typename NormalizedType::value_type;
      return transformValue<glm::vec<N, NormalizedT>>(
          normalize<N, T>(value),
          this->offset(),
          this->scale());
    } else if constexpr (IsMetadataArray<ElementType>::value) {
      using ArrayElementType = typename MetadataArrayType<ElementType>::type;
      if constexpr (IsMetadataScalar<ArrayElementType>::value) {
        return transformNormalizedArray<ArrayElementType>(
            propertyValueCopyToView(value),
            this->offset(),
            this->scale());
      } else if constexpr (IsMetadataVecN<ArrayElementType>::value) {
        constexpr glm::length_t N = ArrayElementType::length();
        using T = typename ArrayElementType::value_type;
        return transformNormalizedVecNArray<N, T>(
            propertyValueCopyToView(value),
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
   * If this property has a specified "no data" value, the raw value will
   * still be returned, even if it equals the "no data" value.
   *
   * @param u The u-component of the texture coordinates.
   * @param v The v-component of the texture coordinates.
   *
   * @return The value at the nearest pixel to the texture coordinates.
   */

  PropertyValueViewToCopy<ElementType>
  getRaw(double u, double v) const noexcept {
    CESIUM_ASSERT(
        this->_status == PropertyTexturePropertyViewStatus::Valid &&
        "Check the status() first to make sure view is valid");

    std::vector<uint8_t> sample =
        this->sampleNearestPixel(u, v, this->_channels);

    return assembleValueFromChannels<ElementType>(
        std::span(sample.data(), this->_channels.size()));
  }

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
  std::vector<int64_t> _channels;
  std::string _swizzle;
};
#pragma endregion

} // namespace CesiumGltf
