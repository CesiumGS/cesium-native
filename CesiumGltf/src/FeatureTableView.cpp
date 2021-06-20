#include "CesiumGltf/FeatureTableView.h"

namespace CesiumGltf {
template <typename T>
static bool checkOffsetBuffer(
    const gsl::span<const std::byte>& offsetBuffer,
    size_t valueBufferSize,
    size_t instanceCount,
    bool checkBitSize) {
  if (offsetBuffer.size() % sizeof(T) != 0) {
    return false;
  }

  size_t size = offsetBuffer.size() / sizeof(T);
  if (size != instanceCount + 1) {
    return false;
  }

  gsl::span<const T> offsetValues(
      reinterpret_cast<const T*>(offsetBuffer.data()),
      size);

  for (size_t i = 1; i < offsetValues.size(); ++i) {
    if (offsetValues[i] < offsetValues[i - 1]) {
      return false;
    }
  }

  if (!checkBitSize) {
    return offsetValues.back() <= valueBufferSize;
  }

  return offsetValues.back() / 8 <= valueBufferSize;
}

template <typename T>
static bool checkStringArrayOffsetBuffer(
    const gsl::span<const std::byte>& arrayOffsetBuffer,
    const gsl::span<const std::byte>& stringOffsetBuffer,
    size_t valueBufferSize,
    size_t instanceCount) {
  if (!checkOffsetBuffer<T>(
          arrayOffsetBuffer,
          stringOffsetBuffer.size(),
          instanceCount,
          false)) {
    return false;
  }

  const T* value = reinterpret_cast<const T*>(arrayOffsetBuffer.data());
  if (!checkOffsetBuffer<T>(
          stringOffsetBuffer,
          valueBufferSize,
          value[instanceCount] / sizeof(T),
          false)) {
    return false;
  }

  return true;
}

FeatureTableView::FeatureTableView(
    const Model* model,
    const FeatureTable* featureTable)
    : _model{model}, _featureTable{featureTable}, _class{nullptr} {
  const ModelEXT_feature_metadata* metadata =
      model->getExtension<ModelEXT_feature_metadata>();
  assert(
      metadata != nullptr &&
      "Model must contain ModelEXT_feature_metadata to use FeatureTableView");

  const std::optional<Schema>& schema = metadata->schema;
  assert(
      schema != std::nullopt &&
      "ModelEXT_feature_metadata must contain Schema to use FeatureTableView");

  auto classIter =
      schema->classes.find(_featureTable->classProperty.value_or(""));
  if (classIter != schema->classes.end()) {
    _class = &classIter->second;
  }
}

const ClassProperty*
FeatureTableView::getClassProperty(const std::string& propertyName) const {
  if (_class == nullptr) {
    return nullptr;
  }

  auto propertyIter = _class->properties.find(propertyName);
  if (propertyIter == _class->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}

gsl::span<const std::byte>
FeatureTableView::getBufferSafe(int32_t bufferViewIdx) const {
  const BufferView* bufferView =
      _model->getSafe(&_model->bufferViews, bufferViewIdx);
  if (!bufferView) {
    return {};
  }

  const Buffer* buffer = _model->getSafe(&_model->buffers, bufferView->buffer);
  if (!buffer) {
    return {};
  }

  if (bufferView->byteOffset % 8 != 0) {
    return {};
  }

  if (bufferView->byteOffset + bufferView->byteLength >
      static_cast<int64_t>(buffer->cesium.data.size())) {
    return {};
  }

  return gsl::span<const std::byte>(
      buffer->cesium.data.data() + bufferView->byteOffset,
      static_cast<size_t>(bufferView->byteLength));
}

gsl::span<const std::byte> FeatureTableView::getOffsetBufferSafe(
    int32_t bufferViewIdx,
    PropertyType offsetType,
    size_t valueBufferSize,
    size_t instanceCount,
    bool checkBitsSize) const {
  gsl::span<const std::byte> offsetBuffer = getBufferSafe(bufferViewIdx);
  if (offsetBuffer.empty()) {
    return {};
  }

  bool isValid = false;
  switch (offsetType) {
  case PropertyType::Uint8:
    isValid = checkOffsetBuffer<uint8_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint16:
    isValid = checkOffsetBuffer<uint16_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint32:
    isValid = checkOffsetBuffer<uint32_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  case PropertyType::Uint64:
    isValid = checkOffsetBuffer<uint64_t>(
        offsetBuffer,
        valueBufferSize,
        instanceCount,
        checkBitsSize);
    break;
  default:
    break;
  }

  if (!isValid) {
    return {};
  }

  return offsetBuffer;
}

std::optional<PropertyView<bool>> FeatureTableView::getBooleanPropertyValues(
    const ClassProperty* classProperty,
    const FeatureTableProperty& featureTableProperty) const {
  if (classProperty->type != "BOOLEAN") {
    return std::nullopt;
  }

  gsl::span<const std::byte> valueBuffer =
      getBufferSafe(featureTableProperty.bufferView);
  if (valueBuffer.empty()) {
    return std::nullopt;
  }

  size_t maxRequiredBytes = static_cast<size_t>(
      glm::ceil(static_cast<double>(_featureTable->count) / 8.0));
  if (valueBuffer.size() < maxRequiredBytes) {
    return std::nullopt;
  }

  return PropertyView<bool>(
      valueBuffer,
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      PropertyType::None,
      0,
      static_cast<size_t>(_featureTable->count));
}

std::optional<PropertyView<std::string_view>>
FeatureTableView::getStringPropertyValues(
    const ClassProperty* classProperty,
    const FeatureTableProperty& featureTableProperty) const {
  if (classProperty->type != "STRING") {
    return std::nullopt;
  }

  gsl::span<const std::byte> valueBuffer =
      getBufferSafe(featureTableProperty.bufferView);
  if (valueBuffer.empty()) {
    return std::nullopt;
  }

  PropertyType offsetType =
      convertOffsetStringToPropertyType(featureTableProperty.offsetType);
  if (offsetType == PropertyType::None) {
    return std::nullopt;
  }

  gsl::span<const std::byte> offsetBuffer = getOffsetBufferSafe(
      featureTableProperty.stringOffsetBufferView,
      offsetType,
      valueBuffer.size(),
      static_cast<size_t>(_featureTable->count),
      false);
  if (offsetBuffer.empty()) {
    return std::nullopt;
  }

  return PropertyView<std::string_view>(
      valueBuffer,
      gsl::span<const std::byte>(),
      offsetBuffer,
      offsetType,
      0,
      static_cast<size_t>(_featureTable->count));
}

std::optional<PropertyView<ArrayView<std::string_view>>>
FeatureTableView::getStringArrayPropertyValues(
    const ClassProperty* classProperty,
    const FeatureTableProperty& featureTableProperty) const {
  if (classProperty->type != "ARRAY") {
    return std::nullopt;
  }

  if (!classProperty->componentType.isString() ||
      classProperty->componentType.getString() != "STRING") {
    return std::nullopt;
  }

  // get value buffer
  gsl::span<const std::byte> valueBuffer =
      getBufferSafe(featureTableProperty.bufferView);
  if (valueBuffer.empty()) {
    return std::nullopt;
  }

  // check fixed or dynamic array
  int64_t componentCount = classProperty->componentCount.value_or(0);
  if (componentCount > 0 && featureTableProperty.arrayOffsetBufferView >= 0) {
    return std::nullopt;
  }

  if (componentCount <= 0 && featureTableProperty.arrayOffsetBufferView < 0) {
    return std::nullopt;
  }

  // get offset type
  PropertyType offsetType =
      convertOffsetStringToPropertyType(featureTableProperty.offsetType);
  if (offsetType == PropertyType::None) {
    return std::nullopt;
  }

  // get string offset buffer
  if (featureTableProperty.stringOffsetBufferView < 0) {
    return std::nullopt;
  }

  // fixed array
  if (componentCount > 0) {
    gsl::span<const std::byte> stringOffsetBuffer = getOffsetBufferSafe(
        featureTableProperty.stringOffsetBufferView,
        offsetType,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count * componentCount),
        false);
    if (stringOffsetBuffer.empty()) {
      return std::nullopt;
    }

    return PropertyView<ArrayView<std::string_view>>(
        valueBuffer,
        gsl::span<const std::byte>(),
        stringOffsetBuffer,
        offsetType,
        static_cast<size_t>(componentCount),
        static_cast<size_t>(_featureTable->count));
  }

  // dynamic array
  gsl::span<const std::byte> stringOffsetBuffer =
      getBufferSafe(featureTableProperty.stringOffsetBufferView);
  if (stringOffsetBuffer.empty()) {
    return std::nullopt;
  }

  gsl::span<const std::byte> arrayOffsetBuffer =
      getBufferSafe(featureTableProperty.arrayOffsetBufferView);
  if (arrayOffsetBuffer.empty()) {
    return std::nullopt;
  }

  bool isValid = false;
  switch (offsetType) {
  case PropertyType::Uint8:
    isValid = checkStringArrayOffsetBuffer<uint8_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count));
    break;
  case PropertyType::Uint16:
    isValid = checkStringArrayOffsetBuffer<uint16_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count));
    break;
  case PropertyType::Uint32:
    isValid = checkStringArrayOffsetBuffer<uint32_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count));
    break;
  case PropertyType::Uint64:
    isValid = checkStringArrayOffsetBuffer<uint64_t>(
        arrayOffsetBuffer,
        stringOffsetBuffer,
        valueBuffer.size(),
        static_cast<size_t>(_featureTable->count));
    break;
  default:
    break;
  }

  if (!isValid) {
    return std::nullopt;
  }

  return PropertyView<ArrayView<std::string_view>>(
      valueBuffer,
      arrayOffsetBuffer,
      stringOffsetBuffer,
      offsetType,
      0,
      static_cast<size_t>(_featureTable->count));
}
} // namespace CesiumGltf
