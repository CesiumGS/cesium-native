#include "CesiumGltf/StructuralMetadataPropertyTableView.h"

namespace CesiumGltf {
namespace StructuralMetadata {

template <typename T>
static MetadataPropertyViewStatus checkOffsetsBuffer(
    const gsl::span<const std::byte>& offsetBuffer,
    size_t valueBufferSize,
    size_t instanceCount,
    bool checkBitSize,
    MetadataPropertyViewStatus offsetsNotSortedError,
    MetadataPropertyViewStatus offsetOutOfBoundsError) noexcept {
  if (offsetBuffer.size() % sizeof(T) != 0) {
    return MetadataPropertyViewStatus::
        ErrorBufferViewSizeNotDivisibleByTypeSize;
  }

  const size_t size = offsetBuffer.size() / sizeof(T);
  if (size != instanceCount + 1) {
    return MetadataPropertyViewStatus::
        ErrorBufferViewSizeDoesNotMatchPropertyTableCount;
  }

  const gsl::span<const T> offsetValues(
      reinterpret_cast<const T*>(offsetBuffer.data()),
      size);

  for (size_t i = 1; i < offsetValues.size(); ++i) {
    if (offsetValues[i] < offsetValues[i - 1]) {
      return offsetsNotSortedError;
    }
  }

  if (checkBitSize) {
    if (offsetValues.back() / 8 <= valueBufferSize) {
      return MetadataPropertyViewStatus::Valid;
    }

    return offsetOutOfBoundsError;
  }

  if (offsetValues.back() <= valueBufferSize) {
    return MetadataPropertyViewStatus::Valid;
  }

  return offsetOutOfBoundsError;
}

template <typename T>
static MetadataPropertyViewStatus checkStringAndArrayOffsetsBuffers(
    const gsl::span<const std::byte>& arrayOffsets,
    const gsl::span<const std::byte>& stringOffsets,
    size_t valueBufferSize,
    PropertyComponentType stringOffsetType,
    size_t propertyTableCount) noexcept {
  const auto status = checkOffsetsBuffer<T>(
      arrayOffsets,
      stringOffsets.size(),
      propertyTableCount,
      false,
      MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted,
      MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);

  if (status != MetadataPropertyViewStatus::Valid) {
    return status;
  }

  const T* pValue = reinterpret_cast<const T*>(arrayOffsets.data());

  switch (stringOffsetType) {
  case PropertyComponentType::Uint8:
    return checkOffsetsBuffer<uint8_t>(
        stringOffsets,
        valueBufferSize,
        pValue[propertyTableCount] / sizeof(T),
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint16:
    return checkOffsetsBuffer<uint16_t>(
        stringOffsets,
        valueBufferSize,
        pValue[propertyTableCount] / sizeof(T),
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint32:
    return checkOffsetsBuffer<uint32_t>(
        stringOffsets,
        valueBufferSize,
        pValue[propertyTableCount] / sizeof(T),
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint64:
    return checkOffsetsBuffer<uint64_t>(
        stringOffsets,
        valueBufferSize,
        pValue[propertyTableCount] / sizeof(T),
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  default:
    return MetadataPropertyViewStatus::ErrorInvalidStringOffsetType;
  }
}

MetadataPropertyTableView::MetadataPropertyTableView(
    const Model& model,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable)
    : _pModel{&model},
      _pPropertyTable{&propertyTable},
      _pClass{nullptr},
      _status() {
  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  if (!pMetadata) {
    _status =
        MetadataPropertyTableViewStatus::ErrorNoStructuralMetadataExtension;
    return;
  }

  const std::optional<ExtensionExtStructuralMetadataSchema>& schema =
      pMetadata->schema;
  if (!schema) {
    _status = MetadataPropertyTableViewStatus::ErrorNoSchema;
    return;
  }

  auto classIter = schema->classes.find(_pPropertyTable->classProperty);
  if (classIter != schema->classes.end()) {
    _pClass = &classIter->second;
  }

  if (!_pClass) {
    _status =
        MetadataPropertyTableViewStatus::ErrorPropertyTableClassDoesNotExist;
  }
}

const ExtensionExtStructuralMetadataClassProperty*
MetadataPropertyTableView::getClassProperty(
    const std::string& propertyName) const {
  if (_status != MetadataPropertyTableViewStatus::Valid) {
    return nullptr;
  }

  auto propertyIter = _pClass->properties.find(propertyName);
  if (propertyIter == _pClass->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}

MetadataPropertyViewStatus MetadataPropertyTableView::getBufferSafe(
    int32_t bufferViewIdx,
    gsl::span<const std::byte>& buffer) const noexcept {
  buffer = {};

  const BufferView* pBufferView =
      _pModel->getSafe(&_pModel->bufferViews, bufferViewIdx);
  if (!pBufferView) {
    return MetadataPropertyViewStatus::ErrorInvalidValueBufferView;
  }

  const Buffer* pBuffer =
      _pModel->getSafe(&_pModel->buffers, pBufferView->buffer);
  if (!pBuffer) {
    return MetadataPropertyViewStatus::ErrorInvalidValueBuffer;
  }

  if (pBufferView->byteOffset + pBufferView->byteLength >
      static_cast<int64_t>(pBuffer->cesium.data.size())) {
    return MetadataPropertyViewStatus::ErrorBufferViewOutOfBounds;
  }

  buffer = gsl::span<const std::byte>(
      pBuffer->cesium.data.data() + pBufferView->byteOffset,
      static_cast<size_t>(pBufferView->byteLength));
  return MetadataPropertyViewStatus::Valid;
}

MetadataPropertyViewStatus MetadataPropertyTableView::getArrayOffsetsBufferSafe(
    int32_t arrayOffsetsBufferView,
    PropertyComponentType arrayOffsetType,
    size_t valueBufferSize,
    size_t propertyTableCount,
    bool checkBitsSize,
    gsl::span<const std::byte>& arrayOffsetsBuffer) const noexcept {
  const MetadataPropertyViewStatus bufferStatus =
      getBufferSafe(arrayOffsetsBufferView, arrayOffsetsBuffer);
  if (bufferStatus != MetadataPropertyViewStatus::Valid) {
    return bufferStatus;
  }

  switch (arrayOffsetType) {
  case PropertyComponentType::Uint8:
    return checkOffsetsBuffer<uint8_t>(
        arrayOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        checkBitsSize,
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  case PropertyComponentType::Uint16:
    return checkOffsetsBuffer<uint16_t>(
        arrayOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        checkBitsSize,
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  case PropertyComponentType::Uint32:
    return checkOffsetsBuffer<uint32_t>(
        arrayOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        checkBitsSize,
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  case PropertyComponentType::Uint64:
    return checkOffsetsBuffer<uint64_t>(
        arrayOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        checkBitsSize,
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  default:
    return MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType;
  }
}

MetadataPropertyViewStatus
MetadataPropertyTableView::getStringOffsetsBufferSafe(
    int32_t stringOffsetsBufferView,
    PropertyComponentType stringOffsetType,
    size_t valueBufferSize,
    size_t propertyTableCount,
    gsl::span<const std::byte>& stringOffsetsBuffer) const noexcept {
  const MetadataPropertyViewStatus bufferStatus =
      getBufferSafe(stringOffsetsBufferView, stringOffsetsBuffer);
  if (bufferStatus != MetadataPropertyViewStatus::Valid) {
    return bufferStatus;
  }

  switch (stringOffsetType) {
  case PropertyComponentType::Uint8:
    return checkOffsetsBuffer<uint8_t>(
        stringOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint16:
    return checkOffsetsBuffer<uint16_t>(
        stringOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint32:
    return checkOffsetsBuffer<uint32_t>(
        stringOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  case PropertyComponentType::Uint64:
    return checkOffsetsBuffer<uint64_t>(
        stringOffsetsBuffer,
        valueBufferSize,
        propertyTableCount,
        false,
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted,
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  default:
    return MetadataPropertyViewStatus::ErrorInvalidStringOffsetType;
  }
}

MetadataPropertyView<std::string_view>
MetadataPropertyTableView::getStringPropertyValues(
    const ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTableProperty&
        propertyTableProperty) const {
  if (classProperty.array) {
    return createInvalidPropertyView<std::string_view>(
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  if (classProperty.type !=
      ExtensionExtStructuralMetadataClassProperty::Type::STRING) {
    return createInvalidPropertyView<std::string_view>(
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  gsl::span<const std::byte> values;
  auto status = getBufferSafe(propertyTableProperty.values, values);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<std::string_view>(status);
  }

  const PropertyComponentType offsetType =
      convertStringOffsetTypeStringToPropertyComponentType(
          propertyTableProperty.stringOffsetType);
  if (offsetType == PropertyComponentType::None) {
    return createInvalidPropertyView<std::string_view>(
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);
  }

  gsl::span<const std::byte> stringOffsets;
  status = getStringOffsetsBufferSafe(
      propertyTableProperty.stringOffsets,
      offsetType,
      values.size(),
      static_cast<size_t>(_pPropertyTable->count),
      stringOffsets);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<std::string_view>(status);
  }

  return MetadataPropertyView<std::string_view>(
      MetadataPropertyViewStatus::Valid,
      values,
      gsl::span<const std::byte>(),
      stringOffsets,
      PropertyComponentType::None,
      offsetType,
      0,
      _pPropertyTable->count,
      classProperty.normalized);
}

MetadataPropertyView<MetadataArrayView<std::string_view>>
MetadataPropertyTableView::getStringArrayPropertyValues(
    const ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTableProperty&
        propertyTableProperty) const {
  if (!classProperty.array) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  if (classProperty.type !=
      ExtensionExtStructuralMetadataClassProperty::Type::STRING) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  gsl::span<const std::byte> values;
  auto status = getBufferSafe(propertyTableProperty.values, values);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  // Check if array is fixed or variable length
  const int64_t fixedLengthArrayCount = classProperty.count.value_or(0);
  if (fixedLengthArrayCount > 0 && propertyTableProperty.arrayOffsets >= 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }

  if (fixedLengthArrayCount <= 0 && propertyTableProperty.arrayOffsets < 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }

  // Get string offset type
  const PropertyComponentType stringOffsetType =
      convertStringOffsetTypeStringToPropertyComponentType(
          propertyTableProperty.stringOffsetType);
  if (stringOffsetType == PropertyComponentType::None) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);
  }

  if (propertyTableProperty.stringOffsets < 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetBufferView);
  }

  // Handle fixed-length arrays
  if (fixedLengthArrayCount > 0) {
    gsl::span<const std::byte> stringOffsets;
    status = getStringOffsetsBufferSafe(
        propertyTableProperty.stringOffsets,
        stringOffsetType,
        values.size(),
        static_cast<size_t>(_pPropertyTable->count * fixedLengthArrayCount),
        stringOffsets);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
          status);
    }

    return MetadataPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::Valid,
        values,
        gsl::span<const std::byte>(),
        stringOffsets,
        PropertyComponentType::None,
        stringOffsetType,
        fixedLengthArrayCount,
        _pPropertyTable->count,
        classProperty.normalized);
  }

  // Get array offset type
  const PropertyComponentType arrayOffsetType =
      convertArrayOffsetTypeStringToPropertyComponentType(
          propertyTableProperty.arrayOffsetType);
  if (arrayOffsetType == PropertyComponentType::None) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  if (propertyTableProperty.arrayOffsets < 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetBufferView);
  }

  // Handle variable-length arrays
  gsl::span<const std::byte> stringOffsets;
  status = getBufferSafe(propertyTableProperty.stringOffsets, stringOffsets);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  gsl::span<const std::byte> arrayOffsets;
  status = getBufferSafe(propertyTableProperty.arrayOffsets, arrayOffsets);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  switch (arrayOffsetType) {
  case PropertyComponentType::Uint8:
    status = checkStringAndArrayOffsetsBuffers<uint8_t>(
        arrayOffsets,
        stringOffsets,
        values.size(),
        stringOffsetType,
        static_cast<size_t>(_pPropertyTable->count));
    break;
  case PropertyComponentType::Uint16:
    status = checkStringAndArrayOffsetsBuffers<uint16_t>(
        arrayOffsets,
        stringOffsets,
        values.size(),
        stringOffsetType,
        static_cast<size_t>(_pPropertyTable->count));
    break;
  case PropertyComponentType::Uint32:
    status = checkStringAndArrayOffsetsBuffers<uint32_t>(
        arrayOffsets,
        stringOffsets,
        values.size(),
        stringOffsetType,
        static_cast<size_t>(_pPropertyTable->count));
    break;
  case PropertyComponentType::Uint64:
    status = checkStringAndArrayOffsetsBuffers<uint64_t>(
        arrayOffsets,
        stringOffsets,
        values.size(),
        stringOffsetType,
        static_cast<size_t>(_pPropertyTable->count));
    break;
  default:
    status = MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType;
    break;
  }

  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  return MetadataPropertyView<MetadataArrayView<std::string_view>>(
      MetadataPropertyViewStatus::Valid,
      values,
      arrayOffsets,
      stringOffsets,
      arrayOffsetType,
      stringOffsetType,
      0,
      _pPropertyTable->count,
      classProperty.normalized);
}

} // namespace StructuralMetadata
} // namespace CesiumGltf
