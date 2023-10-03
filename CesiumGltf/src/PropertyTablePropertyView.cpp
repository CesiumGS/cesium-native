#include "CesiumGltf/PropertyTablePropertyView.h"

namespace CesiumGltf {

// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize;
const PropertyViewStatusType PropertyTablePropertyViewStatus::
    ErrorBufferViewSizeDoesNotMatchPropertyTableCount;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetOutOfBounds;

int64_t getOffsetTypeSize(PropertyComponentType offsetType) noexcept {
  switch (offsetType) {
  case PropertyComponentType::Uint8:
    return sizeof(uint8_t);
  case PropertyComponentType::Uint16:
    return sizeof(uint16_t);
  case PropertyComponentType::Uint32:
    return sizeof(uint32_t);
  case PropertyComponentType::Uint64:
    return sizeof(uint64_t);
  default:
    return 0;
  }
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, false>::PropertyTablePropertyView()
    : PropertyView<ElementType, false>(),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0},
      _stringOffsets{},
      _stringOffsetType{PropertyComponentType::None},
      _stringOffsetTypeSize{0} {}

template <typename ElementType>
PropertyTablePropertyView<ElementType, false>::PropertyTablePropertyView(
    PropertyViewStatusType status)
    : PropertyView<ElementType, false>(status),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0},
      _stringOffsets{},
      _stringOffsetType{PropertyComponentType::None},
      _stringOffsetTypeSize{0} {
  assert(
      this->_status != PropertyTablePropertyViewStatus::Valid &&
      "An empty property view should not be constructed with a valid status");
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, false>::PropertyTablePropertyView(
    const ClassProperty& classProperty,
    int64_t size)
    : PropertyView<ElementType, false>(classProperty),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0},
      _stringOffsets{},
      _stringOffsetType{PropertyComponentType::None},
      _stringOffsetTypeSize{0} {
  if (this->_status != PropertyTablePropertyViewStatus::Valid) {
    // Don't override the status / size if something is wrong with the class
    // property's definition.
    return;
  }

  if (!classProperty.defaultProperty) {
    // This constructor should only be called if the class property *has* a
    // default value. But in the case that it does not, this property view
    // becomes invalid.
    this->_status = PropertyTablePropertyViewStatus::ErrorNonexistentProperty;
    return;
  }

  this->_status = PropertyTablePropertyViewStatus::EmptyPropertyWithDefault;
  this->_size = size;
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, false>::PropertyTablePropertyView(
    const PropertyTableProperty& property,
    const ClassProperty& classProperty,
    int64_t size,
    gsl::span<const std::byte> values) noexcept
    : PropertyView<ElementType>(classProperty, property),
      _values{values},
      _size{this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0},
      _stringOffsets{},
      _stringOffsetType{PropertyComponentType::None},
      _stringOffsetTypeSize{0} {}

template <typename ElementType>
PropertyTablePropertyView<ElementType, false>::PropertyTablePropertyView(
    const PropertyTableProperty& property,
    const ClassProperty& classProperty,
    int64_t size,
    gsl::span<const std::byte> values,
    gsl::span<const std::byte> arrayOffsets,
    gsl::span<const std::byte> stringOffsets,
    PropertyComponentType arrayOffsetType,
    PropertyComponentType stringOffsetType) noexcept
    : PropertyView<ElementType>(classProperty, property),
      _values{values},
      _size{this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
      _arrayOffsets{arrayOffsets},
      _arrayOffsetType{arrayOffsetType},
      _arrayOffsetTypeSize{getOffsetTypeSize(arrayOffsetType)},
      _stringOffsets{stringOffsets},
      _stringOffsetType{stringOffsetType},
      _stringOffsetTypeSize{getOffsetTypeSize(stringOffsetType)} {}

template <typename ElementType>
std::optional<ElementType> PropertyTablePropertyView<ElementType, false>::get(
    int64_t index) const noexcept {
  if (this->_status ==
      PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
    assert(index >= 0 && "index must be non-negative");
    assert(index < size() && "index must be less than size");

    return this->defaultValue();
  }

  ElementType value = getRaw(index);

  if (value == this->noData()) {
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

template <typename ElementType>
ElementType PropertyTablePropertyView<ElementType, false>::getRaw(
    int64_t index) const noexcept {
  assert(
      this->_status == PropertyTablePropertyViewStatus::Valid &&
      "Check the status() first to make sure view is valid");
  assert(
      size() > 0 && "Check the size() of the view to make sure it's not empty");
  assert(index >= 0 && "index must be non-negative");
  assert(index < size() && "index must be less than size");

  if constexpr (IsMetadataNumeric<ElementType>::value) {
    return getNumericValue(index);
  }

  if constexpr (IsMetadataBoolean<ElementType>::value) {
    return getBooleanValue(index);
  }

  if constexpr (IsMetadataString<ElementType>::value) {
    return getStringValue(index);
  }

  if constexpr (IsMetadataNumericArray<ElementType>::value) {
    return getNumericArrayValues<typename MetadataArrayType<ElementType>::type>(
        index);
  }

  if constexpr (IsMetadataBooleanArray<ElementType>::value) {
    return getBooleanArrayValues(index);
  }

  if constexpr (IsMetadataStringArray<ElementType>::value) {
    return getStringArrayValues(index);
  }
}

template <typename ElementType>
int64_t PropertyTablePropertyView<ElementType, false>::size() const noexcept {
  return _size;
}

template <typename ElementType>
ElementType PropertyTablePropertyView<ElementType, false>::getNumericValue(
    int64_t index) const noexcept {
  return reinterpret_cast<const ElementType*>(_values.data())[index];
}

template <typename ElementType>
bool PropertyTablePropertyView<ElementType, false>::getBooleanValue(
    int64_t index) const noexcept {
  const int64_t byteIndex = index / 8;
  const int64_t bitIndex = index % 8;
  const int bitValue =
      static_cast<int>(_values[size_t(byteIndex)] >> bitIndex) & 1;
  return bitValue == 1;
}

template <typename ElementType>
std::string_view PropertyTablePropertyView<ElementType, false>::getStringValue(
    int64_t index) const noexcept {
  const size_t currentOffset = getOffsetFromOffsetsBuffer(
      size_t(index),
      _stringOffsets,
      _stringOffsetType);
  const size_t nextOffset = getOffsetFromOffsetsBuffer(
      size_t(index + 1),
      _stringOffsets,
      _stringOffsetType);
  return std::string_view(
      reinterpret_cast<const char*>(_values.data() + currentOffset),
      nextOffset - currentOffset);
}

template <typename ElementType>
template <typename T>
PropertyArrayView<T>
PropertyTablePropertyView<ElementType, false>::getNumericArrayValues(
    int64_t index) const noexcept {
  size_t count = static_cast<size_t>(this->arrayCount());
  // Handle fixed-length arrays
  if (count > 0) {
    size_t arraySize = count * sizeof(T);
    const gsl::span<const std::byte> values(
        _values.data() + size_t(index) * arraySize,
        arraySize);
    return PropertyArrayView<T>{values};
  }

  // Handle variable-length arrays
  const size_t currentOffset = getOffsetFromOffsetsBuffer(
      size_t(index),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t nextOffset = getOffsetFromOffsetsBuffer(
      size_t(index + 1),
      _arrayOffsets,
      _arrayOffsetType);
  const gsl::span<const std::byte> values(
      _values.data() + currentOffset,
      nextOffset - currentOffset);
  return PropertyArrayView<T>{values};
}

template <typename ElementType>
PropertyArrayView<std::string_view>
PropertyTablePropertyView<ElementType, false>::getStringArrayValues(
    int64_t index) const noexcept {
  size_t count = static_cast<size_t>(this->arrayCount());
  // Handle fixed-length arrays
  if (count > 0) {
    // Copy the corresponding string offsets to pass to the PropertyArrayView.
    const size_t arraySize = count * size_t(_stringOffsetTypeSize);
    const gsl::span<const std::byte> stringOffsetValues(
        _stringOffsets.data() + size_t(index) * arraySize,
        arraySize + size_t(_stringOffsetTypeSize));
    return PropertyArrayView<std::string_view>(
        _values,
        stringOffsetValues,
        _stringOffsetType,
        int64_t(count));
  }

  // Handle variable-length arrays
  const size_t currentArrayOffset = getOffsetFromOffsetsBuffer(
      size_t(index),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t nextArrayOffset = getOffsetFromOffsetsBuffer(
      size_t(index + 1),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t arraySize = nextArrayOffset - currentArrayOffset;
  const gsl::span<const std::byte> stringOffsetValues(
      _stringOffsets.data() + currentArrayOffset,
      arraySize + size_t(_arrayOffsetTypeSize));
  return PropertyArrayView<std::string_view>(
      _values,
      stringOffsetValues,
      _stringOffsetType,
      int64_t(arraySize) / _arrayOffsetTypeSize);
}

template <typename ElementType>
PropertyArrayView<bool>
PropertyTablePropertyView<ElementType, false>::getBooleanArrayValues(
    int64_t index) const noexcept {
  size_t count = static_cast<size_t>(this->arrayCount());
  // Handle fixed-length arrays
  if (count > 0) {
    const size_t offsetBits = count * size_t(index);
    const size_t nextOffsetBits = count * size_t(index + 1);
    const gsl::span<const std::byte> buffer(
        _values.data() + offsetBits / 8,
        (nextOffsetBits / 8 - offsetBits / 8 + 1));
    return PropertyArrayView<bool>(buffer, offsetBits % 8, int64_t(count));
  }

  // Handle variable-length arrays
  const size_t currentOffset = getOffsetFromOffsetsBuffer(
      size_t(index),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t nextOffset = getOffsetFromOffsetsBuffer(
      size_t(index + 1),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t totalBits = nextOffset - currentOffset;
  const gsl::span<const std::byte> buffer(
      _values.data() + currentOffset / 8,
      (nextOffset / 8 - currentOffset / 8 + 1));
  return PropertyArrayView<bool>(buffer, currentOffset % 8, int64_t(totalBits));
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, true>::PropertyTablePropertyView()
    : PropertyView<ElementType, true>(),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0} {}

template <typename ElementType>
PropertyTablePropertyView<ElementType, true>::PropertyTablePropertyView(
    PropertyViewStatusType status)
    : PropertyView<ElementType, true>(status),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0} {
  assert(
      this->_status != PropertyTablePropertyViewStatus::Valid &&
      "An empty property view should not be constructed with a valid status");
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, true>::PropertyTablePropertyView(
    const ClassProperty& classProperty,
    int64_t size)
    : PropertyView<ElementType, true>(classProperty),
      _values{},
      _size{0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0} {
  if (this->_status != PropertyTablePropertyViewStatus::Valid) {
    // Don't override the status / size if something is wrong with the class
    // property's definition.
    return;
  }

  if (!classProperty.defaultProperty) {
    // This constructor should only be called if the class property *has* a
    // default value. But in the case that it does not, this property view
    // becomes invalid.
    this->_status = PropertyTablePropertyViewStatus::ErrorNonexistentProperty;
    return;
  }

  this->_status = PropertyTablePropertyViewStatus::EmptyPropertyWithDefault;
  this->_size = size;
}

template <typename ElementType>
PropertyTablePropertyView<ElementType, true>::PropertyTablePropertyView(
    const PropertyTableProperty& property,
    const ClassProperty& classProperty,
    int64_t size,
    gsl::span<const std::byte> values) noexcept
    : PropertyView<ElementType, true>(classProperty, property),
      _values{values},
      _size{this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
      _arrayOffsets{},
      _arrayOffsetType{PropertyComponentType::None},
      _arrayOffsetTypeSize{0} {}

template <typename ElementType>
PropertyTablePropertyView<ElementType, true>::PropertyTablePropertyView(
    const PropertyTableProperty& property,
    const ClassProperty& classProperty,
    int64_t size,
    gsl::span<const std::byte> values,
    gsl::span<const std::byte> arrayOffsets,
    PropertyComponentType arrayOffsetType) noexcept
    : PropertyView<ElementType, true>(classProperty, property),
      _values{values},
      _size{this->_status == PropertyTablePropertyViewStatus::Valid ? size : 0},
      _arrayOffsets{arrayOffsets},
      _arrayOffsetType{arrayOffsetType},
      _arrayOffsetTypeSize{getOffsetTypeSize(arrayOffsetType)} {}

template <typename ElementType>
std::optional<
    typename PropertyTablePropertyView<ElementType, true>::NormalizedType>
PropertyTablePropertyView<ElementType, true>::get(
    int64_t index) const noexcept {
  if (this->_status ==
      PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
    assert(index >= 0 && "index must be non-negative");
    assert(index < size() && "index must be less than size");

    return this->defaultValue();
  }

  ElementType value = getRaw(index);
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

  if constexpr (IsMetadataMatN<ElementType>::value) {
    constexpr glm::length_t N = ElementType::length();
    using T = typename ElementType::value_type;
    using NormalizedT = typename NormalizedType::value_type;
    return transformValue<glm::mat<N, N, NormalizedT>>(
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

    if constexpr (IsMetadataMatN<ArrayElementType>::value) {
      constexpr glm::length_t N = ArrayElementType::length();
      using T = typename ArrayElementType::value_type;
      return transformNormalizedMatNArray<N, T>(
          value,
          this->offset(),
          this->scale());
    }
  }
}

template <typename ElementType>
ElementType PropertyTablePropertyView<ElementType, true>::getRaw(
    int64_t index) const noexcept {
  assert(
      this->_status == PropertyTablePropertyViewStatus::Valid &&
      "Check the status() first to make sure view is valid");
  assert(
      size() > 0 && "Check the size() of the view to make sure it's not empty");
  assert(index >= 0 && "index must be non-negative");
  assert(index < size() && "index must be less than size");

  if constexpr (IsMetadataNumeric<ElementType>::value) {
    return getValue(index);
  }

  if constexpr (IsMetadataNumericArray<ElementType>::value) {
    return getArrayValues<typename MetadataArrayType<ElementType>::type>(index);
  }
}

template <typename ElementType>
int64_t PropertyTablePropertyView<ElementType, true>::size() const noexcept {
  return this->_status == PropertyTablePropertyViewStatus::Valid ? _size : 0;
}

template <typename ElementType>
ElementType PropertyTablePropertyView<ElementType, true>::getValue(
    int64_t index) const noexcept {
  return reinterpret_cast<const ElementType*>(_values.data())[index];
}

template <typename ElementType>
template <typename T>
PropertyArrayView<T>
PropertyTablePropertyView<ElementType, true>::getArrayValues(
    int64_t index) const noexcept {
  size_t count = static_cast<size_t>(this->arrayCount());
  // Handle fixed-length arrays
  if (count > 0) {
    size_t arraySize = count * sizeof(T);
    const gsl::span<const std::byte> values(
        _values.data() + size_t(index) * arraySize,
        arraySize);
    return PropertyArrayView<T>{values};
  }

  // Handle variable-length arrays
  const size_t currentOffset = getOffsetFromOffsetsBuffer(
      size_t(index),
      _arrayOffsets,
      _arrayOffsetType);
  const size_t nextOffset = getOffsetFromOffsetsBuffer(
      size_t(index + 1),
      _arrayOffsets,
      _arrayOffsetType);
  const gsl::span<const std::byte> values(
      _values.data() + currentOffset,
      nextOffset - currentOffset);
  return PropertyArrayView<T>{values};
}

template class PropertyTablePropertyView<int8_t, false>;
template class PropertyTablePropertyView<uint8_t, false>;
template class PropertyTablePropertyView<int16_t, false>;
template class PropertyTablePropertyView<uint16_t, false>;
template class PropertyTablePropertyView<int32_t, false>;
template class PropertyTablePropertyView<uint32_t, false>;
template class PropertyTablePropertyView<int64_t, false>;
template class PropertyTablePropertyView<uint64_t, false>;
template class PropertyTablePropertyView<float>;
template class PropertyTablePropertyView<double>;
template class PropertyTablePropertyView<bool>;
template class PropertyTablePropertyView<std::string_view>;
template class PropertyTablePropertyView<glm::vec<2, int8_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, uint8_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, int16_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, uint16_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, int32_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, uint32_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, int64_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, uint64_t>, false>;
template class PropertyTablePropertyView<glm::vec<2, float>>;
template class PropertyTablePropertyView<glm::vec<2, double>>;
template class PropertyTablePropertyView<glm::vec<3, int8_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, uint8_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, int16_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, uint16_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, int32_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, uint32_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, int64_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, uint64_t>, false>;
template class PropertyTablePropertyView<glm::vec<3, float>>;
template class PropertyTablePropertyView<glm::vec<3, double>>;
template class PropertyTablePropertyView<glm::vec<4, int8_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, uint8_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, int16_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, uint16_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, int32_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, uint32_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, int64_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, uint64_t>, false>;
template class PropertyTablePropertyView<glm::vec<4, float>>;
template class PropertyTablePropertyView<glm::vec<4, double>>;
template class PropertyTablePropertyView<glm::mat<2, 2, int8_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, int16_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint16_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, int32_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint32_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, int64_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint64_t>, false>;
template class PropertyTablePropertyView<glm::mat<2, 2, float>>;
template class PropertyTablePropertyView<glm::mat<2, 2, double>>;
template class PropertyTablePropertyView<glm::mat<3, 3, int8_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, int16_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint16_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, int32_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint32_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, int64_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint64_t>, false>;
template class PropertyTablePropertyView<glm::mat<3, 3, float>>;
template class PropertyTablePropertyView<glm::mat<3, 3, double>>;
template class PropertyTablePropertyView<glm::mat<4, 4, int8_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, int16_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint16_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, int32_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint32_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, int64_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint64_t>, false>;
template class PropertyTablePropertyView<glm::mat<4, 4, float>>;
template class PropertyTablePropertyView<glm::mat<4, 4, double>>;
template class PropertyTablePropertyView<PropertyArrayView<int8_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<uint8_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<int16_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<uint16_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<int32_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<uint32_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<int64_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<uint64_t>, false>;
template class PropertyTablePropertyView<PropertyArrayView<float>>;
template class PropertyTablePropertyView<PropertyArrayView<double>>;
template class PropertyTablePropertyView<PropertyArrayView<bool>>;
template class PropertyTablePropertyView<PropertyArrayView<std::string_view>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint64_t>>,
    false>;
template class PropertyTablePropertyView<PropertyArrayView<glm::vec<2, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, double>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint64_t>>,
    false>;
template class PropertyTablePropertyView<PropertyArrayView<glm::vec<3, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, double>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint64_t>>,
    false>;
template class PropertyTablePropertyView<PropertyArrayView<glm::vec<4, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, double>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, double>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, double>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint8_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint16_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint32_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint64_t>>,
    false>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, float>>>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, double>>>;

template class PropertyTablePropertyView<int8_t, true>;
template class PropertyTablePropertyView<uint8_t, true>;
template class PropertyTablePropertyView<int16_t, true>;
template class PropertyTablePropertyView<uint16_t, true>;
template class PropertyTablePropertyView<int32_t, true>;
template class PropertyTablePropertyView<uint32_t, true>;
template class PropertyTablePropertyView<int64_t, true>;
template class PropertyTablePropertyView<uint64_t, true>;
template class PropertyTablePropertyView<glm::vec<2, int8_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, uint8_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, int16_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, uint16_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, int32_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, uint32_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, int64_t>, true>;
template class PropertyTablePropertyView<glm::vec<2, uint64_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, int8_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, uint8_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, int16_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, uint16_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, int32_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, uint32_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, int64_t>, true>;
template class PropertyTablePropertyView<glm::vec<3, uint64_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, int8_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, uint8_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, int16_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, uint16_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, int32_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, uint32_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, int64_t>, true>;
template class PropertyTablePropertyView<glm::vec<4, uint64_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, int8_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint8_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, int16_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint16_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, int32_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint32_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, int64_t>, true>;
template class PropertyTablePropertyView<glm::mat<2, 2, uint64_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, int8_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint8_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, int16_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint16_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, int32_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint32_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, int64_t>, true>;
template class PropertyTablePropertyView<glm::mat<3, 3, uint64_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, int8_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint8_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, int16_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint16_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, int32_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint32_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, int64_t>, true>;
template class PropertyTablePropertyView<glm::mat<4, 4, uint64_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<int8_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<uint8_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<int16_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<uint16_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<int32_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<uint32_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<int64_t>, true>;
template class PropertyTablePropertyView<PropertyArrayView<uint64_t>, true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<2, uint64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<3, uint64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::vec<4, uint64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<2, 2, uint64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<3, 3, uint64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint8_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint16_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint32_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, int64_t>>,
    true>;
template class PropertyTablePropertyView<
    PropertyArrayView<glm::mat<4, 4, uint64_t>>,
    true>;

} // namespace CesiumGltf
