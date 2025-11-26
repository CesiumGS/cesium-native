#pragma once

#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/Enum.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyAttributeProperty.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>

namespace CesiumGltf {

/**
 * @brief The type used for fields of \ref PropertyViewStatus.
 */
typedef int32_t PropertyViewStatusType;

/**
 * @brief Indicates the status of a property view.
 *
 * The {@link PropertyView} constructor always completes successfully.
 * However, there may be fundamental errors with the property definition. In
 * such cases, this enumeration provides the reason.
 *
 * This is defined with a class of static consts as opposed to an enum, so that
 * derived property view classes can extend the statuses with their own specific
 * errors.
 */
class PropertyViewStatus {
public:
  /**
   * @brief This property view is valid and ready to use.
   */
  static const PropertyViewStatusType Valid = 0;

  /**
   * @brief This property view does not contain any data, but specifies a
   * default value. This happens when a class property is defined with a default
   * value and omitted from an instance of the class's collective properties. In
   * this case, it is not possible to retrieve the raw data from a property, but
   * its default value will be accessible.
   */
  static const PropertyViewStatusType EmptyPropertyWithDefault = 1;

  /**
   * @brief This property view is trying to view a property that does not
   * exist.
   */
  static const PropertyViewStatusType ErrorNonexistentProperty = 2;

  /**
   * @brief This property view's type does not match what is
   * specified in {@link ClassProperty::type}.
   */
  static const PropertyViewStatusType ErrorTypeMismatch = 3;

  /**
   * @brief This property view's component type does not match what
   * is specified in {@link ClassProperty::componentType}.
   */
  static const PropertyViewStatusType ErrorComponentTypeMismatch = 4;

  /**
   * @brief This property view differs from what is specified in
   * {@link ClassProperty::array}.
   */
  static const PropertyViewStatusType ErrorArrayTypeMismatch = 5;

  /**
   * @brief This property says it is normalized, but it does not have an integer
   * component type.
   */
  static const PropertyViewStatusType ErrorInvalidNormalization = 6;

  /**
   * @brief This property view's normalization differs from what
   * is specified in {@link ClassProperty::normalized}
   */
  static const PropertyViewStatusType ErrorNormalizationMismatch = 7;

  /**
   * @brief The property provided an invalid offset value.
   */
  static const PropertyViewStatusType ErrorInvalidOffset = 8;

  /**
   * @brief The property provided an invalid scale value.
   */
  static const PropertyViewStatusType ErrorInvalidScale = 9;

  /**
   * @brief The property provided an invalid maximum value.
   */
  static const PropertyViewStatusType ErrorInvalidMax = 10;

  /**
   * @brief The property provided an invalid minimum value.
   */
  static const PropertyViewStatusType ErrorInvalidMin = 11;

  /**
   * @brief The property provided an invalid "no data" value.
   */
  static const PropertyViewStatusType ErrorInvalidNoDataValue = 12;

  /**
   * @brief The property provided an invalid default value.
   */
  static const PropertyViewStatusType ErrorInvalidDefaultValue = 13;

  /**
   * @brief The property provided an invalid enum value.
   */
  static const PropertyViewStatusType ErrorInvalidEnum = 14;
};

/**
 * @brief Validates a \ref ClassProperty representing a property, checking for
 * any type mismatches.
 *
 * @tparam T The value type of the PropertyView that was constructed for this
 * ClassProperty.
 * @param classProperty The class property to validate.
 * @param pEnumDefinition If the class property is an enum, this should be the
 * enum definition. If not, this should be nullptr.
 *
 * @returns A \ref PropertyViewStatus value representing the error found while
 * validating, or \ref PropertyViewStatus::Valid if no errors were found.
 */
template <typename T>
PropertyViewStatusType validatePropertyType(
    const ClassProperty& classProperty,
    const CesiumGltf::Enum* pEnumDefinition = nullptr) {

  if (!canRepresentPropertyType<T>(
          convertStringToPropertyType(classProperty.type))) {
    return PropertyViewStatus::ErrorTypeMismatch;
  }

  const bool isEnum = classProperty.type == ClassProperty::Type::ENUM;
  const bool hasEnumDefinition = pEnumDefinition != nullptr;
  if (isEnum != hasEnumDefinition) {
    return PropertyViewStatus::ErrorInvalidEnum;
  }

  PropertyComponentType expectedComponentType =
      TypeToPropertyType<T>::component;
  if (isEnum) {
    if (expectedComponentType !=
        convertStringToPropertyComponentType(pEnumDefinition->valueType)) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }
  } else {
    if (!classProperty.componentType &&
        expectedComponentType != PropertyComponentType::None) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }

    if (classProperty.componentType &&
        expectedComponentType != convertStringToPropertyComponentType(
                                     *classProperty.componentType)) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }
  }

  if (classProperty.array) {
    return PropertyViewStatus::ErrorArrayTypeMismatch;
  }

  return PropertyViewStatus::Valid;
}

/**
 * @brief Validates a \ref ClassProperty representing an array of values,
 * checking for any type mismatches.
 *
 * @tparam T The array type of the PropertyView that was constructed for this
 * ClassProperty.
 * @param classProperty The class property to validate.
 * @param pEnumDefinition If the class property is an enum array, this should be
 * the enum definition. If not, this should be nullptr.
 *
 * @returns A \ref PropertyViewStatus value representing the error found while
 * validating, or \ref PropertyViewStatus::Valid if no errors were found.
 */
template <typename T>
PropertyViewStatusType validateArrayPropertyType(
    const ClassProperty& classProperty,
    const CesiumGltf::Enum* pEnumDefinition = nullptr) {
  using ElementType = typename MetadataArrayType<T>::type;

  if (!canRepresentPropertyType<ElementType>(
          convertStringToPropertyType(classProperty.type))) {
    return PropertyViewStatus::ErrorTypeMismatch;
  }

  const bool isEnum = classProperty.type == ClassProperty::Type::ENUM;
  const bool hasEnumDefinition = pEnumDefinition != nullptr;
  if (isEnum != hasEnumDefinition) {
    return PropertyViewStatus::ErrorInvalidEnum;
  }

  PropertyComponentType expectedComponentType =
      TypeToPropertyType<ElementType>::component;
  if (isEnum) {
    if (expectedComponentType !=
        convertStringToPropertyComponentType(pEnumDefinition->valueType)) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }
  } else {
    if (!classProperty.componentType &&
        expectedComponentType != PropertyComponentType::None) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }

    if (classProperty.componentType &&
        expectedComponentType != convertStringToPropertyComponentType(
                                     *classProperty.componentType)) {
      return PropertyViewStatus::ErrorComponentTypeMismatch;
    }
  }

  if (!classProperty.array) {
    return PropertyViewStatus::ErrorArrayTypeMismatch;
  }

  return PropertyViewStatus::Valid;
}

/**
 * @brief Attempts to get a scalar value from the provided \ref
 * CesiumUtility::JsonValue "JsonValue".
 *
 * @param jsonValue The value to attempt to get as a scalar.
 * @returns A scalar of type `T` if successful, or `std::nullopt` if not.
 */
template <typename T>
static std::optional<T> getScalar(const CesiumUtility::JsonValue& jsonValue) {
  return jsonValue.getSafeNumber<T>();
}

/**
 * @brief Attempts to obtain a vector of type `VecType` from the provided \ref
 * CesiumUtility::JsonValue "JsonValue".
 *
 * @param jsonValue The value to attempt to get as a vector. To be successful,
 * this \ref CesiumUtility::JsonValue "JsonValue" must be an array with the same
 * number of elements as `VecType`.
 * @returns A vector of type `VecType` if successful, or `std::nullopt` if not.
 */
template <typename VecType>
static std::optional<VecType>
getVecN(const CesiumUtility::JsonValue& jsonValue) {
  if (!jsonValue.isArray()) {
    return std::nullopt;
  }

  const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
  constexpr glm::length_t N = VecType::length();
  if (array.size() != N) {
    return std::nullopt;
  }

  using T = typename VecType::value_type;

  VecType result;
  for (glm::length_t i = 0; i < N; i++) {
    std::optional<T> value = getScalar<T>(array[static_cast<size_t>(i)]);
    if (!value) {
      return std::nullopt;
    }

    result[i] = *value;
  }

  return result;
}

/**
 * @brief Attempts to obtain a matrix of type `MatType` from the provided \ref
 * CesiumUtility::JsonValue "JsonValue".
 *
 * @param jsonValue The value to attempt to get as a matrix. To be successful,
 * this \ref CesiumUtility::JsonValue "JsonValue" must be an array with the same
 * number of elements as `MatType`. For example, to read a 4x4 matrix, the \ref
 * CesiumUtility::JsonValue "JsonValue" must be an array with 16 elements.
 * @returns A matrix of type `MatType` if successful, or `std::nullopt` if not.
 */
template <typename MatType>
static std::optional<MatType>
getMatN(const CesiumUtility::JsonValue& jsonValue) {
  if (!jsonValue.isArray()) {
    return std::nullopt;
  }

  const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
  constexpr glm::length_t N = MatType::length();
  if (array.size() != static_cast<size_t>(N * N)) {
    return std::nullopt;
  }

  using T = typename MatType::value_type;

  MatType result;
  for (glm::length_t i = 0; i < N; i++) {
    // Try to parse each value in the column.
    for (glm::length_t j = 0; j < N; j++) {
      std::optional<T> value =
          getScalar<T>(array[static_cast<size_t>(i * N + j)]);
      if (!value) {
        return std::nullopt;
      }

      result[i][j] = *value;
    }
  }

  return result;
}

/**
 * @brief Obtains the number of values of type `ElementType` that could fit in
 * the buffer.
 *
 * @param buffer The buffer whose size will be used for this calculation.
 * @returns The number of values of type `ElementType` that could fit in
 * `buffer`. This value will be equivalent to `floor(buffer->size() /
 * sizeof(ElementType))`.
 */
template <typename ElementType>
int64_t getCount(std::optional<std::vector<std::byte>>& buffer) {
  if (!buffer) {
    return 0;
  }

  return static_cast<int64_t>(buffer->size() / sizeof(ElementType));
}

/**
 * @brief Represents a metadata property in EXT_structural_metadata.
 */
template <typename ElementType, bool Normalized = false> class PropertyView;

/**
 * @brief Represents a non-normalized metadata property in
 * EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property, they
 * may be overridden by individual instances of the property themselves. The
 * constructor is responsible for resolving those differences.
 *
 * @tparam ElementType The C++ type of the values in this property
 */
template <typename ElementType> class PropertyView<ElementType, false> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : PropertyView(classProperty, nullptr) {}

  /**
   * @brief Constructs a property instance from a class definition and enum
   * definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const CesiumGltf::Enum* pEnumDefinition)
      : _status(
            validatePropertyType<ElementType>(classProperty, pEnumDefinition)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(convertStringToPropertyType(classProperty.type)) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
      return;
    }

    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(classProperty);
    }

    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!_required && _propertyType == PropertyType::Enum) {
        CESIUM_ASSERT(pEnumDefinition != nullptr);
        if constexpr (IsMetadataInteger<ElementType>::value) {
          // "noData" can only be defined if the property is not required.
          _noData = getEnumValue(*classProperty.noData, *pEnumDefinition);
        }
      } else if (!_required && _propertyType != PropertyType::Enum) {
        _noData = getValue(*classProperty.noData);
      }

      if (!_noData) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required && _propertyType == PropertyType::Enum) {
        CESIUM_ASSERT(pEnumDefinition != nullptr);
        if constexpr (IsMetadataInteger<ElementType>::value) {
          // "default" can only be defined if the property is not required.
          _defaultValue =
              getEnumValue(*classProperty.defaultProperty, *pEnumDefinition);
        }
      } else if (!_required && _propertyType != PropertyType::Enum) {
        _defaultValue = getValue(*classProperty.defaultProperty);
      }

      if (!_defaultValue) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error
   * with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition, with an optional associated enum definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& property,
      const CesiumGltf::Enum* pEnumDefinition = nullptr)
      : PropertyView(classProperty, pEnumDefinition) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(property);
    }
  }

  /**
   * @brief Constructs a property instance from a property texture property,
   * class definition, and an optional associated enum definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& property,
      const CesiumGltf::Enum* pEnumDefinition = nullptr)
      : PropertyView(classProperty, pEnumDefinition) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(property);
    }
  }

  /**
   * @brief Constructs a property instance from a property attribute property
   * and its class definition, along with an optional enum definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyAttributeProperty& property,
      const CesiumGltf::Enum* pEnumDefinition = nullptr)
      : PropertyView(classProperty, pEnumDefinition) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(property);
    }
  }

public:
  /**
   * @brief Gets the status of this property view, indicating whether an error
   * occurred.
   *
   * @return The status of this property view.
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @brief Gets the name of the property being viewed. Returns std::nullopt if
   * no name was specified.
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @brief Gets the semantic of the property being viewed. The semantic is an
   * identifier that describes how this property should be interpreted, and
   * cannot be used by other properties in the class. Returns std::nullopt if no
   * semantic was specified.
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @brief Gets the description of the property being viewed. Returns
   * std::nullopt if no description was specified.
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @brief Get the element count of the fixed-length arrays in this property.
   * Only applicable when the property is an array type.
   *
   * @return The count of this property.
   */
  int64_t arrayCount() const noexcept { return 0; }

  /**
   * @brief Whether this property has a normalized integer type.
   */
  bool normalized() const noexcept { return false; }

  /**
   * @brief Gets the offset to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's offset, or std::nullopt if it was not specified.
   */
  std::optional<ElementType> offset() const noexcept { return _offset; }

  /**
   * @brief Gets the scale to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's scale, or std::nullopt if it was not specified.
   */
  std::optional<ElementType> scale() const noexcept { return _scale; }

  /**
   * @brief Gets the maximum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the maximum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's maximum value, or std::nullopt if it was not
   * specified.
   */
  std::optional<ElementType> max() const noexcept { return _max; }

  /**
   * @brief Gets the minimum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the minimum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's minimum value, or std::nullopt if it was not
   * specified.
   */
  std::optional<ElementType> min() const noexcept { return _min; }

  /**
   * @brief Whether the property must be present in every entity conforming to
   * the class. If not required, instances of the property may include "no data"
   * values, or the entire property may be omitted.
   */
  bool required() const noexcept { return _required; }

  /**
   * @brief Gets the "no data" value, i.e., the value representing missing data
   * in the property wherever it appears. Also known as a sentinel value. This
   * is given as the plain property value, without the transforms from the
   * normalized, offset, and scale properties.
   *
   * @returns The property's "no data" value, or std::nullopt if it was not
   * specified.
   */
  std::optional<ElementType> noData() const noexcept { return _noData; }

  /**
   * @brief Gets the default value to use when encountering a "no data" value or
   * an omitted property. The value is given in its final form, taking the
   * effect of normalized, offset, and scale properties into account.
   *
   * @returns The property's default value, or std::nullopt if it was not
   * specified.
   */
  std::optional<ElementType> defaultValue() const noexcept {
    return _defaultValue;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return _propertyType; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  std::optional<ElementType> _offset;
  std::optional<ElementType> _scale;
  std::optional<ElementType> _max;
  std::optional<ElementType> _min;

  bool _required;
  std::optional<ElementType> _noData;
  std::optional<ElementType> _defaultValue;
  PropertyType _propertyType;

  /**
   * @brief Attempts to parse an ElementType from the given json value.
   *
   * If ElementType is a type with multiple components, e.g. a VECN or MATN
   * type, this will return std::nullopt if one or more components could not be
   * parsed.
   *
   * @return The value as an instance of ElementType, or std::nullopt if it
   * could not be parsed.
   */
  static std::optional<ElementType>
  getValue(const CesiumUtility::JsonValue& jsonValue) {
    if constexpr (IsMetadataScalar<ElementType>::value) {
      return getScalar<ElementType>(jsonValue);
    }

    if constexpr (IsMetadataVecN<ElementType>::value) {
      return getVecN<ElementType>(jsonValue);
    }

    if constexpr (IsMetadataMatN<ElementType>::value) {
      return getMatN<ElementType>(jsonValue);
    }
  }

  static std::optional<ElementType> getEnumValue(
      const CesiumUtility::JsonValue& value,
      const CesiumGltf::Enum& enumDefinition) {
    if (!value.isString()) {
      return std::nullopt;
    }

    const CesiumUtility::JsonValue::String& valueStr = value.getString();
    const auto foundValue = std::find_if(
        enumDefinition.values.begin(),
        enumDefinition.values.end(),
        [&valueStr](const CesiumGltf::EnumValue& enumValue) {
          return enumValue.name == valueStr;
        });

    if (foundValue == enumDefinition.values.end()) {
      return std::nullopt;
    }

    return static_cast<ElementType>(foundValue->value);
  }

  using PropertyDefinitionType = std::variant<
      ClassProperty,
      PropertyTableProperty,
      PropertyTextureProperty,
      PropertyAttributeProperty>;

  /**
   * @brief Attempts to parse offset, scale, min, and max properties from the
   * given property type.
   */
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          if (property.offset) {
            // Only floating point types can specify an offset.
            switch (TypeToPropertyType<ElementType>::component) {
            case PropertyComponentType::Float32:
            case PropertyComponentType::Float64:
              this->_offset = getValue(*property.offset);
              if (this->_offset) {
                break;
              }
              // If it does not break here, something went wrong.
              [[fallthrough]];
            default:
              this->_status = PropertyViewStatus::ErrorInvalidOffset;
              return;
            }
          }

          if (property.scale) {
            // Only floating point types can specify a scale.
            switch (TypeToPropertyType<ElementType>::component) {
            case PropertyComponentType::Float32:
            case PropertyComponentType::Float64:
              this->_scale = getValue(*property.scale);
              if (this->_scale) {
                break;
              }
              // If it does not break here, something went wrong.
              [[fallthrough]];
            default:
              this->_status = PropertyViewStatus::ErrorInvalidScale;
              return;
            }
          }

          if (property.max) {
            this->_max = getValue(*property.max);
            if (!this->_max) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            this->_min = getValue(*property.min);
            if (!this->_min) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }
};

/**
 * @brief Represents a normalized metadata property in
 * EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property, they
 * may be overridden by individual instances of the property themselves. The
 * constructor is responsible for resolving those differences.
 *
 * @tparam ElementType The C++ type of the values in this property. Must have an
 * integer component type.
 */
template <typename ElementType> class PropertyView<ElementType, true> {
private:
  using NormalizedType = typename TypeToNormalizedType<ElementType>::type;

public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validatePropertyType<ElementType>(classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(convertStringToPropertyType(classProperty.type)) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (!classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
    }

    getNumericPropertyValues(classProperty);
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!_required) {
        // "noData" should not be defined if the property is required.
        _noData = getValue<ElementType>(*classProperty.noData);
      }
      if (!_noData) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      // default value should not be defined if the property is required.
      if (!_required) {
        _defaultValue =
            getValue<NormalizedType>(*classProperty.defaultProperty);
      }
      if (!_defaultValue) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& property)
      : PropertyView(classProperty) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    getNumericPropertyValues(property);
  }

  /**
   * @brief Constructs a property instance from a property texture property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& property)
      : PropertyView(classProperty) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    getNumericPropertyValues(property);
  }

  /**
   * @brief Constructs a property instance from a property attribute property
   * and its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyAttributeProperty& property)
      : PropertyView(classProperty) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    getNumericPropertyValues(property);
  }

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return true; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<NormalizedType> offset() const noexcept { return _offset; }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<NormalizedType> scale() const noexcept { return _scale; }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<NormalizedType> max() const noexcept { return _max; }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<NormalizedType> min() const noexcept { return _min; }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<ElementType> noData() const noexcept { return _noData; }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<NormalizedType> defaultValue() const noexcept {
    return _defaultValue;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return _propertyType; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  std::optional<NormalizedType> _offset;
  std::optional<NormalizedType> _scale;
  std::optional<NormalizedType> _max;
  std::optional<NormalizedType> _min;

  bool _required;
  std::optional<ElementType> _noData;
  std::optional<NormalizedType> _defaultValue;
  PropertyType _propertyType;

  /**
   * @brief Attempts to parse from the given json value.
   *
   * If T is a type with multiple components, e.g. a VECN or MATN type, this
   * will return std::nullopt if one or more components could not be parsed.
   *
   * @return The value as an instance of T, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static std::optional<T> getValue(const CesiumUtility::JsonValue& jsonValue) {
    if constexpr (IsMetadataScalar<T>::value) {
      return getScalar<T>(jsonValue);
    }

    if constexpr (IsMetadataVecN<T>::value) {
      return getVecN<T>(jsonValue);
    }

    if constexpr (IsMetadataMatN<T>::value) {
      return getMatN<T>(jsonValue);
    }
  }

  using PropertyDefinitionType = std::variant<
      ClassProperty,
      PropertyTableProperty,
      PropertyTextureProperty,
      PropertyAttributeProperty>;

  /**
   * @brief Attempts to parse offset, scale, min, and max properties from the
   * given property type.
   */
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          if (property.offset) {
            _offset = getValue<NormalizedType>(*property.offset);
            if (!_offset) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidOffset;
              return;
            }
          }

          if (property.scale) {
            _scale = getValue<NormalizedType>(*property.scale);
            if (!_scale) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidScale;
              return;
            }
          }

          if (property.max) {
            _max = getValue<NormalizedType>(*property.max);
            if (!_scale) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            _min = getValue<NormalizedType>(*property.min);
            if (!_scale) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }
};

/**
 * @brief Represents a boolean metadata property in
 * EXT_structural_metadata.
 */
template <> class PropertyView<bool> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _required(false),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validatePropertyType<bool>(classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _required(classProperty.required),
        _defaultValue(std::nullopt) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue = getBooleanValue(*classProperty.defaultProperty);
      }

      if (!_defaultValue) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _required(false),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/,
      const CesiumGltf::Enum* /*pEnumDefinition*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return false; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<bool> offset() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<bool> scale() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<bool> max() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<bool> min() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<bool> noData() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<bool> defaultValue() const noexcept { return _defaultValue; }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return PropertyType::Boolean; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  bool _required;
  std::optional<bool> _defaultValue;

  static std::optional<bool>
  getBooleanValue(const CesiumUtility::JsonValue& value) {
    if (!value.isBool()) {
      return std::nullopt;
    }

    return value.getBool();
  }
};

/**
 * @brief Represents a string metadata property in
 * EXT_structural_metadata.
 */
template <> class PropertyView<std::string_view> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validatePropertyType<std::string_view>(classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!_required) {
        _noData = getStringValue(*classProperty.noData);
      }

      if (!_noData) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue = getStringValue(*classProperty.defaultProperty);
      }

      if (!_defaultValue) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/,
      const CesiumGltf::Enum* /*pEnumDefinition*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return false; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<std::string_view> offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<std::string_view> scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<std::string_view> max() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<std::string_view> min() const noexcept { return std::nullopt; }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<std::string_view> noData() const noexcept {
    if (_noData)
      return std::string_view(*_noData);

    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<std::string_view> defaultValue() const noexcept {
    if (_defaultValue)
      return std::string_view(*_defaultValue);

    return std::nullopt;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return PropertyType::String; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  bool _required;
  std::optional<std::string> _noData;
  std::optional<std::string> _defaultValue;

  static std::optional<std::string>
  getStringValue(const CesiumUtility::JsonValue& value) {
    if (!value.isString()) {
      return std::nullopt;
    }

    return std::string(value.getString().c_str());
  }
};

/**
 * @brief Represents a non-normalized array metadata property in
 * EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property, they
 * may be overridden by individual instances of the property themselves. The
 * constructor is responsible for resolving those differences.
 *
 * @tparam ElementType The C++ type of the elements in the array values for this
 * property.
 */
template <typename ElementType>
class PropertyView<PropertyArrayView<ElementType>, false> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(false),
        _noData(),
        _defaultValue(),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : PropertyView(classProperty, nullptr) {}

  /**
   * @brief Constructs a property instance from a class definition and enum
   * definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const CesiumGltf::Enum* pEnumDefinition)
      : _status(validateArrayPropertyType<PropertyArrayView<ElementType>>(
            classProperty,
            pEnumDefinition)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(classProperty.count ? *classProperty.count : 0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(classProperty.required),
        _noData(),
        _defaultValue(),
        _propertyType(convertStringToPropertyType(classProperty.type)) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
      return;
    }

    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(classProperty);
    }

    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!this->_required && this->_propertyType == PropertyType::Enum) {
        CESIUM_ASSERT(pEnumDefinition != nullptr);
        if constexpr (IsMetadataInteger<ElementType>::value) {
          this->_noData =
              getEnumArrayValue(*classProperty.noData, *pEnumDefinition);
        }
      } else if (!this->_required) {
        this->_noData = getArrayValue(*classProperty.noData);
      }

      if (this->_noData.size() == 0) {
        // The value was specified but something went wrong.
        this->_status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!this->_required && this->_propertyType == PropertyType::Enum) {
        CESIUM_ASSERT(pEnumDefinition != nullptr);
        if constexpr (IsMetadataInteger<ElementType>::value) {
          this->_defaultValue = getEnumArrayValue(
              *classProperty.defaultProperty,
              *pEnumDefinition);
        }
      } else if (!this->_required) {
        this->_defaultValue = getArrayValue(*classProperty.defaultProperty);
      }

      if (this->_defaultValue.size() == 0) {
        // The value was specified but something went wrong.
        this->_status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(false),
        _noData(),
        _defaultValue(),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a property table property,
   * its class definition, and an optional enum definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& property,
      const CesiumGltf::Enum* pEnumDefinition = nullptr)
      : PropertyView(classProperty, pEnumDefinition) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(property);
    }
  }

  /**
   * @brief Constructs a property instance from a property texture property,
   * its class definition, and an optional enum definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& property,
      const CesiumGltf::Enum* pEnumDefinition = nullptr)
      : PropertyView(classProperty, pEnumDefinition) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (classProperty.type != ClassProperty::Type::ENUM) {
      getNumericPropertyValues(property);
    }
  }

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return false; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<PropertyArrayView<ElementType>> offset() const noexcept {
    return this->_offset.size() > 0 ? std::make_optional(this->_offset.view())
                                    : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<ElementType>> scale() const noexcept {
    return this->_scale.size() > 0 ? std::make_optional(this->_scale.view())
                                   : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<ElementType>> max() const noexcept {
    return this->_max.size() > 0 ? std::make_optional(this->_max.view())
                                 : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<ElementType>> min() const noexcept {
    return this->_min.size() > 0 ? std::make_optional(this->_min.view())
                                 : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<ElementType>> noData() const noexcept {
    return this->_noData.size() > 0 ? std::make_optional(this->_noData.view())
                                    : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<ElementType>> defaultValue() const noexcept {
    return this->_defaultValue.size() > 0
               ? std::make_optional(this->_defaultValue.view())
               : std::nullopt;
  }

  /**
   * @brief Returns the \ref PropertyType of the property that this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return _propertyType; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;

  PropertyArrayCopy<ElementType> _offset;
  PropertyArrayCopy<ElementType> _scale;
  PropertyArrayCopy<ElementType> _max;
  PropertyArrayCopy<ElementType> _min;

  bool _required;
  PropertyArrayCopy<ElementType> _noData;
  PropertyArrayCopy<ElementType> _defaultValue;
  PropertyType _propertyType;

  using PropertyDefinitionType = std::
      variant<ClassProperty, PropertyTableProperty, PropertyTextureProperty>;
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          bool isFixedSizeArray = this->_count > 0;

          if (property.offset) {
            // Only floating point types can specify an offset.
            switch (TypeToPropertyType<ElementType>::component) {
            case PropertyComponentType::Float32:
            case PropertyComponentType::Float64:
              if (isFixedSizeArray) {
                this->_offset = getArrayValue(*property.offset);
                if (this->_offset.size() == this->_count) {
                  break;
                }
              }
              // If it does not break here, something went wrong.
              [[fallthrough]];
            default:
              this->_status = PropertyViewStatus::ErrorInvalidOffset;
              return;
            }
          }

          if (property.scale) {
            // Only floating point types can specify a scale.
            switch (TypeToPropertyType<ElementType>::component) {
            case PropertyComponentType::Float32:
            case PropertyComponentType::Float64:
              if (isFixedSizeArray) {
                this->_scale = getArrayValue(*property.scale);
                if (this->_scale.size() == this->_count) {
                  break;
                }
              }
              // If it does not break here, something went wrong.
              [[fallthrough]];
            default:
              this->_status = PropertyViewStatus::ErrorInvalidScale;
              return;
            }
          }

          if (property.max) {
            if (isFixedSizeArray) {
              this->_max = getArrayValue(*property.max);
            }
            if (!isFixedSizeArray || this->_max.size() != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            if (isFixedSizeArray) {
              this->_min = getArrayValue(*property.min);
            }
            if (!isFixedSizeArray || this->_min.size() != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }

  static PropertyArrayCopy<ElementType>
  getArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return PropertyArrayCopy<ElementType>();
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<ElementType> values(array.size());

    if constexpr (IsMetadataScalar<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> maybeValue =
            getScalar<ElementType>(array[i]);
        if (!maybeValue.has_value()) {
          return PropertyArrayCopy<ElementType>();
        }

        values[i] = *maybeValue;
      }
    }
    if constexpr (IsMetadataVecN<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> maybeValue = getVecN<ElementType>(array[i]);
        if (!maybeValue.has_value()) {
          return PropertyArrayCopy<ElementType>();
        }

        values[i] = *maybeValue;
      }
    }

    if constexpr (IsMetadataMatN<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> maybeValue = getMatN<ElementType>(array[i]);
        if (!maybeValue.has_value()) {
          return PropertyArrayCopy<ElementType>();
        }

        values[i] = *maybeValue;
      }
    }

    return PropertyArrayCopy<ElementType>(values);
  }

  static PropertyArrayCopy<ElementType> getEnumArrayValue(
      const CesiumUtility::JsonValue& jsonValue,
      const CesiumGltf::Enum& enumDefinition) {
    if (!jsonValue.isArray()) {
      return PropertyArrayCopy<ElementType>();
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<ElementType> values(array.size());

    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isString()) {
        return PropertyArrayCopy<ElementType>();
      }

      // default and noData values for enums contain the name of the enum as a
      // string
      CesiumUtility::JsonValue::String str = array[i].getString();
      auto foundValue = std::find_if(
          enumDefinition.values.begin(),
          enumDefinition.values.end(),
          [&str](const CesiumGltf::EnumValue& enumValue) {
            return enumValue.name == str;
          });

      if (foundValue == enumDefinition.values.end()) {
        return PropertyArrayCopy<ElementType>();
      }

      values[i] = static_cast<ElementType>(foundValue->value);
    }

    return PropertyArrayCopy<ElementType>(values);
  }
};

/**
 * @brief Represents a normalized array metadata property in
 * EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property,
 * they may be overridden by individual instances of the property themselves.
 * The constructor is responsible for resolving those differences.
 *
 * @tparam ElementType The C++ type of the elements in the array values for
 * this property. Must have an integer component type.
 */
template <typename ElementType>
class PropertyView<PropertyArrayView<ElementType>, true> {
private:
  using NormalizedType = typename TypeToNormalizedType<ElementType>::type;

public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(false),
        _noData(),
        _defaultValue(),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validateArrayPropertyType<PropertyArrayView<ElementType>>(
            classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(classProperty.count ? *classProperty.count : 0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(classProperty.required),
        _noData(),
        _defaultValue(),
        _propertyType(convertStringToPropertyType(classProperty.type)) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (!classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
      return;
    }

    getNumericPropertyValues(classProperty);
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!this->_required) {
        this->_noData = getArrayValue<ElementType>(*classProperty.noData);
      }

      if (this->_noData.size() == 0) {
        this->_status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!this->_required) {
        this->_defaultValue =
            getArrayValue<NormalizedType>(*classProperty.defaultProperty);
      }

      if (this->_defaultValue.size() == 0) {
        this->_status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _offset(),
        _scale(),
        _max(),
        _min(),
        _required(false),
        _noData(),
        _defaultValue(),
        _propertyType(PropertyType::Invalid) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& property)
      : PropertyView(classProperty) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    getNumericPropertyValues(property);
  }

  /**
   * @brief Constructs a property instance from a property texture property
   * and its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& property)
      : PropertyView(classProperty) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    getNumericPropertyValues(property);
  }

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return true; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<PropertyArrayView<NormalizedType>> offset() const noexcept {
    return this->_offset.size() > 0 ? std::make_optional(this->_offset.view())
                                    : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<NormalizedType>> scale() const noexcept {
    return this->_scale.size() > 0 ? std::make_optional(this->_scale.view())
                                   : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<NormalizedType>> max() const noexcept {
    return this->_max.size() > 0 ? std::make_optional(this->_max.view())
                                 : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<NormalizedType>> min() const noexcept {
    return this->_min.size() > 0 ? std::make_optional(this->_min.view())
                                 : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<ElementType>> noData() const noexcept {
    return this->_noData.size() > 0 ? std::make_optional(this->_noData.view())
                                    : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<NormalizedType>>
  defaultValue() const noexcept {
    return this->_defaultValue.size() > 0
               ? std::make_optional(this->_defaultValue.view())
               : std::nullopt;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return _propertyType; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;

  PropertyArrayCopy<NormalizedType> _offset;
  PropertyArrayCopy<NormalizedType> _scale;
  PropertyArrayCopy<NormalizedType> _max;
  PropertyArrayCopy<NormalizedType> _min;

  bool _required;
  PropertyArrayCopy<ElementType> _noData;
  PropertyArrayCopy<NormalizedType> _defaultValue;
  PropertyType _propertyType;

  using PropertyDefinitionType = std::
      variant<ClassProperty, PropertyTableProperty, PropertyTextureProperty>;
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          bool isFixedSizeArray = this->_count > 0;
          if (property.offset) {
            if (isFixedSizeArray) {
              this->_offset = getArrayValue<NormalizedType>(*property.offset);
            }
            if (!isFixedSizeArray || this->_offset.size() != this->_count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidOffset;
              return;
            }
          }

          if (property.scale) {
            if (isFixedSizeArray) {
              this->_scale = getArrayValue<NormalizedType>(*property.scale);
            }
            if (!isFixedSizeArray || this->_scale.size() != this->_count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidScale;
              return;
            }
          }

          if (property.max) {
            if (isFixedSizeArray) {
              this->_max = getArrayValue<NormalizedType>(*property.max);
            }
            if (!isFixedSizeArray || this->_max.size() != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            if (isFixedSizeArray) {
              this->_min = getArrayValue<NormalizedType>(*property.min);
            }
            if (!isFixedSizeArray || this->_min.size() != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }

  template <typename T>
  static PropertyArrayCopy<T>
  getArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return PropertyArrayCopy<T>();
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<T> values(array.size());

    if constexpr (IsMetadataScalar<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> maybeElement = getScalar<T>(array[i]);
        if (!maybeElement.has_value()) {
          return PropertyArrayCopy<T>();
        }
        values[i] = *maybeElement;
      }
    }

    if constexpr (IsMetadataVecN<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> maybeElement = getVecN<T>(array[i]);
        if (!maybeElement.has_value()) {
          return PropertyArrayCopy<T>();
        }
        values[i] = *maybeElement;
      }
    }

    if constexpr (IsMetadataMatN<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> maybeElement = getMatN<T>(array[i]);
        if (!maybeElement.has_value()) {
          return PropertyArrayCopy<T>();
        }
        values[i] = *maybeElement;
      }
    }

    return PropertyArrayCopy<T>(values);
  }
};

/**
 * @brief Represents a boolean array metadata property in
 * EXT_structural_metadata.
 */
template <> class PropertyView<PropertyArrayView<bool>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _required(false),
        _defaultValue() {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(
            validateArrayPropertyType<PropertyArrayView<bool>>(classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(classProperty.count ? *classProperty.count : 0),
        _required(classProperty.required),
        _defaultValue() {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.defaultProperty) {
      if (!this->_required) {
        this->_defaultValue =
            getBooleanArrayValue(*classProperty.defaultProperty);
      }

      if (this->_defaultValue.size() == 0 ||
          this->_count > 0 && this->_defaultValue.size() != this->_count) {
        this->_status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _required(false),
        _defaultValue() {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/,
      const CesiumGltf::Enum* /*pEnumDefinition*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return false; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<PropertyArrayView<bool>> offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<bool>> scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<bool>> max() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<bool>> min() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<bool>> noData() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<bool>> defaultValue() const noexcept {
    return this->_defaultValue.size() > 0
               ? std::make_optional(this->_defaultValue.view())
               : std::nullopt;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return PropertyType::Boolean; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;
  bool _required;

  PropertyArrayCopy<bool> _defaultValue;

  static PropertyArrayCopy<bool>
  getBooleanArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return PropertyArrayCopy<bool>();
    }
    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();

    std::vector<bool> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isBool()) {
        // The entire array is invalidated; return.
        return PropertyArrayCopy<bool>();
      }
      values[i] = array[i].getBool();
    }
    return values;
  }
};

/**
 * @brief Represents a string array metadata property in
 * EXT_structural_metadata.
 */
template <> class PropertyView<PropertyArrayView<std::string_view>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _required(false),
        _noData(),
        _defaultValue() {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validateArrayPropertyType<PropertyArrayView<std::string_view>>(
            classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(classProperty.count ? *classProperty.count : 0),
        _required(classProperty.required),
        _noData(),
        _defaultValue() {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!this->_required) {
        this->_noData = getStringArrayValue(*classProperty.noData);
      }

      if (this->_noData.size() == 0 ||
          (this->_count > 0 && this->_noData.size() != this->_count)) {
        this->_status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!this->_required) {
        this->_defaultValue =
            getStringArrayValue(*classProperty.defaultProperty);
      }

      if (this->_defaultValue.size() == 0 ||
          (this->_count > 0 && this->_defaultValue.size() != this->_count)) {
        // The value was specified but something went wrong.
        this->_status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs an invalid instance for an erroneous property.
   *
   * @param status The value of {@link PropertyViewStatus} indicating the error with the property.
   */
  PropertyView(PropertyViewStatusType status)
      : _status(status),
        _name(std::nullopt),
        _semantic(std::nullopt),
        _description(std::nullopt),
        _count(0),
        _required(false),
        _noData(),
        _defaultValue() {}

  /**
   * @brief Constructs a property instance from a property table property
   * and its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/,
      const CesiumGltf::Enum* /*pEnumDefinition*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc PropertyView<ElementType, false>::status
   */
  PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc PropertyView<ElementType, false>::name
   */
  const std::optional<std::string>& name() const noexcept { return _name; }

  /**
   * @copydoc PropertyView<ElementType, false>::semantic
   */
  const std::optional<std::string>& semantic() const noexcept {
    return _semantic;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::description
   */
  const std::optional<std::string>& description() const noexcept {
    return _description;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::arrayCount
   */
  int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc PropertyView<ElementType, false>::normalized
   */
  bool normalized() const noexcept { return false; }

  /**
   * @copydoc PropertyView<ElementType, false>::offset
   */
  std::optional<PropertyArrayView<std::string_view>> offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<std::string_view>> scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<std::string_view>> max() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<std::string_view>> min() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<std::string_view>> noData() const noexcept {
    return this->_noData.size() > 0 ? std::make_optional(this->_noData.view())
                                    : std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<std::string_view>>
  defaultValue() const noexcept {
    return this->_defaultValue.size() > 0
               ? std::make_optional(this->_defaultValue.view())
               : std::nullopt;
  }

  /**
   * @brief Returns the \ref PropertyType of the property this view is
   * accessing.
   */
  PropertyType propertyType() const noexcept { return PropertyType::String; }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;
  bool _required;

  PropertyArrayCopy<std::string_view> _noData;
  PropertyArrayCopy<std::string_view> _defaultValue;

  static PropertyArrayCopy<std::string_view>
  getStringArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return PropertyArrayCopy<std::string_view>();
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<std::string> strings(array.size());

    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isString()) {
        // The entire array is invalidated; return.
        return PropertyArrayCopy<std::string_view>();
      }
      strings[i] = array[i].getString();
    }

    return PropertyArrayCopy<std::string_view>(strings);
  }
};

} // namespace CesiumGltf
