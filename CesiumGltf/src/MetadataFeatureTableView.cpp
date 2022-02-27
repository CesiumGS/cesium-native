#include "CesiumGltf/MetadataFeatureTableView.h"

namespace CesiumGltf {
template <typename T>
static MetadataPropertyViewStatus checkOffsetBuffer(
    const gsl::span<const std::byte>& offsetBuffer,
    size_t valueBufferSize,
    size_t instanceCount,
    bool checkBitSize) noexcept {
  if (offsetBuffer.size() % sizeof(T) != 0) {
    return MetadataPropertyViewStatus::
        InvalidBufferViewSizeNotDivisibleByTypeSize;
  }

  const size_t size = offsetBuffer.size() / sizeof(T);
  if (size != instanceCount + 1) {
    return MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount;
  }

  const gsl::span<const T> offsetValues(
      reinterpret_cast<const T*>(offsetBuffer.data()),
      size);

  for (size_t i = 1; i < offsetValues.size(); ++i) {
    if (offsetValues[i] < offsetValues[i - 1]) {
      return MetadataPropertyViewStatus::InvalidOffsetValuesNotSortedAscending;
    }
  }

  if (!checkBitSize) {
    if (offsetValues.back() <= valueBufferSize) {
      return MetadataPropertyViewStatus::Valid;
    } else {
      return MetadataPropertyViewStatus::
          InvalidOffsetValuePointsToOutOfBoundBuffer;
    }
  }

  if (offsetValues.back() / 8 <= valueBufferSize) {
    return MetadataPropertyViewStatus::Valid;
  } else {
    return MetadataPropertyViewStatus::
        InvalidOffsetValuePointsToOutOfBoundBuffer;
  }
}

template <typename T>
static MetadataPropertyViewStatus checkStringArrayOffsetBuffer(
    const gsl::span<const std::byte>& arrayOffsetBuffer,
    const gsl::span<const std::byte>& stringOffsetBuffer,
    size_t valueBufferSize,
    size_t instanceCount) noexcept {
  const auto status = checkOffsetBuffer<T>(
      arrayOffsetBuffer,
      stringOffsetBuffer.size(),
      instanceCount,
      false);
  if (status != MetadataPropertyViewStatus::Valid) {
    return status;
  }

  const T* pValue = reinterpret_cast<const T*>(arrayOffsetBuffer.data());
  return checkOffsetBuffer<T>(
      stringOffsetBuffer,
      valueBufferSize,
      pValue[instanceCount] / sizeof(T),
      false);
}

MetadataFeatureTableView::MetadataFeatureTableView(
    const Model* pModel,
    const FeatureTable* pFeatureTable)
    : _pModel{pModel}, _pFeatureTable{pFeatureTable}, _pClass{nullptr} {
  assert(pModel != nullptr && "model must not be nullptr");
  assert(pFeatureTable != nullptr && "featureTable must not be nullptr");

  const ExtensionModelExtFeatureMetadata* pMetadata =
      pModel->getExtension<ExtensionModelExtFeatureMetadata>();
  assert(
      pMetadata != nullptr &&
      "Model must contain ExtensionModelExtFeatureMetadata to use "
      "FeatureTableView");

  const std::optional<Schema>& schema = pMetadata->schema;
  assert(
      schema != std::nullopt && "ExtensionModelExtFeatureMetadata must contain "
                                "Schema to use FeatureTableView");

  auto classIter =
      schema->classes.find(_pFeatureTable->classProperty.value_or(""));
  if (classIter != schema->classes.end()) {
    _pClass = &classIter->second;
  }
}

const ClassProperty* MetadataFeatureTableView::getClassProperty(
    const std::string& propertyName) const {
  if (_pClass == nullptr) {
    return nullptr;
  }

  auto propertyIter = _pClass->properties.find(propertyName);
  if (propertyIter == _pClass->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}

MetadataPropertyViewStatus MetadataFeatureTableView::getBufferSafe(
    int32_t bufferViewIdx,
    gsl::span<const std::byte>& buffer) const noexcept {
  buffer = {};

  const BufferView* pBufferView =
      _pModel->getSafe(&_pModel->bufferViews, bufferViewIdx);
  if (!pBufferView) {
    return MetadataPropertyViewStatus::InvalidValueBufferViewIndex;
  }

  const Buffer* pBuffer =
      _pModel->getSafe(&_pModel->buffers, pBufferView->buffer);
  if (!pBuffer) {
    return MetadataPropertyViewStatus::InvalidValueBufferIndex;
  }

  // This is technically required for the EXT_feature_metadata spec, but not
  // necessarily required for EXT_mesh_features. Due to the discrepancy between
  // the two specs, a lot of EXT_feature_metadata glTFs fail to be 8-byte
  // aligned. To be forgiving and more compatible, we do not enforce this.
  /*
  if (pBufferView->byteOffset % 8 != 0) {
    return MetadataPropertyViewStatus::InvalidBufferViewNotAligned8Bytes;
  }
  */

  if (pBufferView->byteOffset + pBufferView->byteLength >
      static_cast<int64_t>(pBuffer->cesium.data.size())) {
    return MetadataPropertyViewStatus::InvalidBufferViewOutOfBound;
  }

  buffer = gsl::span<const std::byte>(
      pBuffer->cesium.data.data() + pBufferView->byteOffset,
      static_cast<size_t>(pBufferView->byteLength));
  return MetadataPropertyViewStatus::Valid;
}

MetadataPropertyViewStatus MetadataFeatureTableView::getOffsetBufferSafe(
    int32_t bufferViewIdx,
    PropertyType offsetType,
    size_t valueBufferSize,
    size_t instanceCount,
    bool checkBitsSize,
    gsl::span<const std::byte>& offsetBuffer) const noexcept {
  auto status = getBufferSafe(bufferViewIdx, offsetBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return status;
  }

  switch (offsetType) {
  case PropertyType::Uint8:
    status = checkOffsetBuffer<uint8_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint16:
    status = checkOffsetBuffer<uint16_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint32:
    status = checkOffsetBuffer<uint32_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint64:
    status = checkOffsetBuffer<uint64_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  default:
    status = MetadataPropertyViewStatus::InvalidOffsetType;
    break;
  }

  return status;
}

MetadataPropertyView<std::string_view>
MetadataFeatureTableView::getStringPropertyValues(
    const ClassProperty& classProperty,
    const FeatureTableProperty& featureTableProperty) const {
  if (classProperty.type != ClassProperty::Type::STRING) {
    return createInvalidPropertyView<std::string_view>(
        MetadataPropertyViewStatus::InvalidTypeMismatch);
  }

  gsl::span<const std::byte> valueBuffer;
  auto status = getBufferSafe(featureTableProperty.bufferView, valueBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<std::string_view>(status);
  }

  const PropertyType offsetType =
      convertOffsetStringToPropertyType(featureTableProperty.offsetType);
  if (offsetType == PropertyType::None) {
    return createInvalidPropertyView<std::string_view>(
        MetadataPropertyViewStatus::InvalidOffsetType);
  }

  gsl::span<const std::byte> offsetBuffer;
  status = getOffsetBufferSafe(
      featureTableProperty.stringOffsetBufferView,
      offsetType,
      valueBuffer.size(),
      static_cast<size_t>(_pFeatureTable->count),
      false,
      offsetBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<std::string_view>(status);
  }

  return MetadataPropertyView<std::string_view>(
      MetadataPropertyViewStatus::Valid,
      valueBuffer,
      gsl::span<const std::byte>(),
      offsetBuffer,
      offsetType,
      0,
      _pFeatureTable->count,
      classProperty.normalized);
}

MetadataPropertyView<MetadataArrayView<std::string_view>>
MetadataFeatureTableView::getStringArrayPropertyValues(
    const ClassProperty& classProperty,
    const FeatureTableProperty& featureTableProperty) const {
  if (classProperty.type != ClassProperty::Type::ARRAY) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::InvalidTypeMismatch);
  }

  if (classProperty.componentType != ClassProperty::ComponentType::STRING) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::InvalidTypeMismatch);
  }

  // get value buffer
  gsl::span<const std::byte> valueBuffer;
  auto status = getBufferSafe(featureTableProperty.bufferView, valueBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  // check fixed or dynamic array
  const int64_t componentCount = classProperty.componentCount.value_or(0);
  if (componentCount > 0 && featureTableProperty.arrayOffsetBufferView >= 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountAndOffsetBufferCoexist);
  }

  if (componentCount <= 0 && featureTableProperty.arrayOffsetBufferView < 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountOrOffsetBufferNotExist);
  }

  // get offset type
  const PropertyType offsetType =
      convertOffsetStringToPropertyType(featureTableProperty.offsetType);
  if (offsetType == PropertyType::None) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::InvalidOffsetType);
  }

  // get string offset buffer
  if (featureTableProperty.stringOffsetBufferView < 0) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::InvalidStringOffsetBufferViewIndex);
  }

  // fixed array
  if (componentCount > 0) {
    gsl::span<const std::byte> stringOffsetBuffer;
    status = getOffsetBufferSafe(
        featureTableProperty.stringOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count * componentCount),
        false,
        stringOffsetBuffer);
    if (status != MetadataPropertyViewStatus::Valid) {
      return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
          status);
    }

    return MetadataPropertyView<MetadataArrayView<std::string_view>>(
        MetadataPropertyViewStatus::Valid,
        valueBuffer,
        gsl::span<const std::byte>(),
        stringOffsetBuffer,
        offsetType,
        componentCount,
        _pFeatureTable->count,
        classProperty.normalized);
  }

  // dynamic array
  gsl::span<const std::byte> stringOffsetBuffer;
  status = getBufferSafe(
      featureTableProperty.stringOffsetBufferView,
      stringOffsetBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  gsl::span<const std::byte> arrayOffsetBuffer;
  status = getBufferSafe(
      featureTableProperty.arrayOffsetBufferView,
      arrayOffsetBuffer);
  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  switch (offsetType) {
  case PropertyType::Uint8:
    status = checkStringArrayOffsetBuffer<uint8_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count));
    break;
  case PropertyType::Uint16:
    status = checkStringArrayOffsetBuffer<uint16_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count));
    break;
  case PropertyType::Uint32:
    status = checkStringArrayOffsetBuffer<uint32_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count));
    break;
  case PropertyType::Uint64:
    status = checkStringArrayOffsetBuffer<uint64_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_pFeatureTable->count));
    break;
  default:
    status = MetadataPropertyViewStatus::InvalidOffsetType;
    break;
  }

  if (status != MetadataPropertyViewStatus::Valid) {
    return createInvalidPropertyView<MetadataArrayView<std::string_view>>(
        status);
  }

  return MetadataPropertyView<MetadataArrayView<std::string_view>>(
      MetadataPropertyViewStatus::Valid,
      valueBuffer,
      arrayOffsetBuffer,
      stringOffsetBuffer,
      offsetType,
      0,
      _pFeatureTable->count,
      classProperty.normalized);
}
} // namespace CesiumGltf
