#pragma once

#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyAttributeProperty.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/PropertyTypeTraits.h>

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
};

/**
 * @brief Validates a \ref ClassProperty, checking for any type mismatches.
 *
 * @returns A \ref PropertyViewStatus value representing the error found while
 * validating, or \ref PropertyViewStatus::Valid if no errors were found.
 */
template <typename T>
PropertyViewStatusType
validatePropertyType(const ClassProperty& classProperty) {
  if (TypeToPropertyType<T>::value !=
      convertStringToPropertyType(classProperty.type)) {
    return PropertyViewStatus::ErrorTypeMismatch;
  }

  PropertyComponentType expectedComponentType =
      TypeToPropertyType<T>::component;

  if (!classProperty.componentType &&
      expectedComponentType != PropertyComponentType::None) {
    return PropertyViewStatus::ErrorComponentTypeMismatch;
  }

  if (classProperty.componentType &&
      expectedComponentType !=
          convertStringToPropertyComponentType(*classProperty.componentType)) {
    return PropertyViewStatus::ErrorComponentTypeMismatch;
  }

  if (classProperty.array) {
    return PropertyViewStatus::ErrorArrayTypeMismatch;
  }

  return PropertyViewStatus::Valid;
}

/**
 * @brief Validates a \ref ClassProperty representing an array, checking for any
 * type mismatches.
 *
 * @returns A \ref PropertyViewStatus value representing the error found while
 * validating, or \ref PropertyViewStatus::Valid if no errors were found.
 */
template <typename T>
PropertyViewStatusType
validateArrayPropertyType(const ClassProperty& classProperty) {
  using ElementType = typename MetadataArrayType<T>::type;
  if (TypeToPropertyType<ElementType>::value !=
      convertStringToPropertyType(classProperty.type)) {
    return PropertyViewStatus::ErrorTypeMismatch;
  }

  PropertyComponentType expectedComponentType =
      TypeToPropertyType<ElementType>::component;

  if (!classProperty.componentType &&
      expectedComponentType != PropertyComponentType::None) {
    return PropertyViewStatus::ErrorComponentTypeMismatch;
  }

  if (classProperty.componentType &&
      expectedComponentType !=
          convertStringToPropertyComponentType(*classProperty.componentType)) {
    return PropertyViewStatus::ErrorComponentTypeMismatch;
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
    std::optional<T> value = getScalar<T>(array[i]);
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
  if (array.size() != N * N) {
    return std::nullopt;
  }

  using T = typename MatType::value_type;

  MatType result;
  for (glm::length_t i = 0; i < N; i++) {
    // Try to parse each value in the column.
    for (glm::length_t j = 0; j < N; j++) {
      std::optional<T> value = getScalar<T>(array[i * N + j]);
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
        _defaultValue(std::nullopt) {}

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
        _defaultValue(std::nullopt) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
      return;
    }

    getNumericPropertyValues(classProperty);
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!_required) {
        // "noData" can only be defined if the property is not required.
        _noData = getValue(*classProperty.noData);
      }

      if (!_noData) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        // "default" can only be defined if the property is not required.
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
        _defaultValue(std::nullopt) {}

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
        _defaultValue(std::nullopt) {}

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
        _defaultValue(std::nullopt) {
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
        _defaultValue(std::nullopt) {}

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
      const PropertyTableProperty& /*property*/)
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
      const PropertyTableProperty& /*property*/)
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
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validateArrayPropertyType<PropertyArrayView<ElementType>>(
            classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(_count = classProperty.count ? *classProperty.count : 0),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorNormalizationMismatch;
      return;
    }

    getNumericPropertyValues(classProperty);
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.noData) {
      if (!_required) {
        _noData = getArrayValue(*classProperty.noData);
      }

      if (!_noData) {
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue = getArrayValue(*classProperty.defaultProperty);
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
        _count(0),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

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
    if (!_offset) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_offset->data(), _offset->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<ElementType>> scale() const noexcept {
    if (!_scale) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_scale->data(), _scale->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<ElementType>> max() const noexcept {
    if (!_max) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_max->data(), _max->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<ElementType>> min() const noexcept {
    if (!_min) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_min->data(), _min->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<ElementType>> noData() const noexcept {
    if (!_noData) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_noData->data(), _noData->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<ElementType>> defaultValue() const noexcept {
    if (!_defaultValue) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(std::span<const std::byte>(
        _defaultValue->data(),
        _defaultValue->size()));
  }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;

  std::optional<std::vector<std::byte>> _offset;
  std::optional<std::vector<std::byte>> _scale;
  std::optional<std::vector<std::byte>> _max;
  std::optional<std::vector<std::byte>> _min;

  bool _required;
  std::optional<std::vector<std::byte>> _noData;
  std::optional<std::vector<std::byte>> _defaultValue;

  using PropertyDefinitionType = std::
      variant<ClassProperty, PropertyTableProperty, PropertyTextureProperty>;
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          if (property.offset) {
            // Only floating point types can specify an offset.
            switch (TypeToPropertyType<ElementType>::component) {
            case PropertyComponentType::Float32:
            case PropertyComponentType::Float64:
              if (this->_count > 0) {
                this->_offset = getArrayValue(*property.offset);
              }
              if (this->_offset &&
                  getCount<ElementType>(this->_offset) == this->_count) {
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
              if (_count > 0) {
                this->_scale = getArrayValue(*property.scale);
              }
              if (this->_scale &&
                  getCount<ElementType>(this->_scale) == this->_count) {
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
            if (this->_count > 0) {
              this->_max = getArrayValue(*property.max);
            }
            if (!this->_max ||
                getCount<ElementType>(this->_max) != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            if (this->_count > 0) {
              this->_min = getArrayValue(*property.min);
            }
            if (!this->_min ||
                getCount<ElementType>(this->_min) != this->_count) {
              // The value was specified but something went wrong.
              this->_status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }

  static std::optional<std::vector<std::byte>>
  getArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return std::nullopt;
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<ElementType> values;
    values.reserve(array.size());

    if constexpr (IsMetadataScalar<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> element = getScalar<ElementType>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }
    if constexpr (IsMetadataVecN<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> element = getVecN<ElementType>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }

    if constexpr (IsMetadataMatN<ElementType>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<ElementType> element = getMatN<ElementType>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }

    std::vector<std::byte> result(values.size() * sizeof(ElementType));
    std::memcpy(result.data(), values.data(), result.size());

    return result;
  }
};

/**
 * @brief Represents a normalized array metadata property in
 * EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property, they
 * may be overridden by individual instances of the property themselves. The
 * constructor is responsible for resolving those differences.
 *
 * @tparam ElementType The C++ type of the elements in the array values for this
 * property. Must have an integer component type.
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
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(validateArrayPropertyType<PropertyArrayView<ElementType>>(
            classProperty)),
        _name(classProperty.name),
        _semantic(classProperty.semantic),
        _description(classProperty.description),
        _count(_count = classProperty.count ? *classProperty.count : 0),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
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
      if (!_required) {
        _noData = getArrayValue<ElementType>(*classProperty.noData);
      }

      if (!_noData) {
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue =
            getArrayValue<NormalizedType>(*classProperty.defaultProperty);
      }

      if (!_defaultValue) {
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
        _count(0),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

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
    if (!_offset) {
      return std::nullopt;
    }

    return PropertyArrayView<NormalizedType>(
        std::span<const std::byte>(_offset->data(), _offset->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::scale
   */
  std::optional<PropertyArrayView<NormalizedType>> scale() const noexcept {
    if (!_scale) {
      return std::nullopt;
    }

    return PropertyArrayView<NormalizedType>(
        std::span<const std::byte>(_scale->data(), _scale->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::max
   */
  std::optional<PropertyArrayView<NormalizedType>> max() const noexcept {
    if (!_max) {
      return std::nullopt;
    }

    return PropertyArrayView<NormalizedType>(
        std::span<const std::byte>(_max->data(), _max->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::min
   */
  std::optional<PropertyArrayView<NormalizedType>> min() const noexcept {
    if (!_min) {
      return std::nullopt;
    }

    return PropertyArrayView<NormalizedType>(
        std::span<const std::byte>(_min->data(), _min->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::required
   */
  bool required() const noexcept { return _required; }

  /**
   * @copydoc PropertyView<ElementType, false>::noData
   */
  std::optional<PropertyArrayView<ElementType>> noData() const noexcept {
    if (!_noData) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        std::span<const std::byte>(_noData->data(), _noData->size()));
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<NormalizedType>>
  defaultValue() const noexcept {
    if (!_defaultValue) {
      return std::nullopt;
    }

    return PropertyArrayView<NormalizedType>(std::span<const std::byte>(
        _defaultValue->data(),
        _defaultValue->size()));
  }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;

  std::optional<std::vector<std::byte>> _offset;
  std::optional<std::vector<std::byte>> _scale;
  std::optional<std::vector<std::byte>> _max;
  std::optional<std::vector<std::byte>> _min;

  bool _required;
  std::optional<std::vector<std::byte>> _noData;
  std::optional<std::vector<std::byte>> _defaultValue;

  using PropertyDefinitionType = std::
      variant<ClassProperty, PropertyTableProperty, PropertyTextureProperty>;
  void getNumericPropertyValues(const PropertyDefinitionType& inProperty) {
    std::visit(
        [this](auto property) {
          if (property.offset) {
            if (_count > 0) {
              _offset = getArrayValue<NormalizedType>(*property.offset);
            }
            if (!_offset || getCount<NormalizedType>(_offset) != _count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidOffset;
              return;
            }
          }

          if (property.scale) {
            if (_count > 0) {
              _scale = getArrayValue<NormalizedType>(*property.scale);
            }
            if (!_scale || getCount<NormalizedType>(_scale) != _count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidScale;
              return;
            }
          }

          if (property.max) {
            if (_count > 0) {
              _max = getArrayValue<NormalizedType>(*property.max);
            }
            if (!_max || getCount<NormalizedType>(_max) != _count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidMax;
              return;
            }
          }

          if (property.min) {
            if (_count > 0) {
              _min = getArrayValue<NormalizedType>(*property.min);
            }
            if (!_min || getCount<NormalizedType>(_min) != _count) {
              // The value was specified but something went wrong.
              _status = PropertyViewStatus::ErrorInvalidMin;
              return;
            }
          }
        },
        inProperty);
  }

  template <typename T>
  static std::optional<std::vector<std::byte>>
  getArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return std::nullopt;
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<T> values;
    values.reserve(array.size());

    if constexpr (IsMetadataScalar<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> element = getScalar<T>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }

    if constexpr (IsMetadataVecN<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> element = getVecN<T>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }

    if constexpr (IsMetadataMatN<T>::value) {
      for (size_t i = 0; i < array.size(); i++) {
        std::optional<T> element = getMatN<T>(array[i]);
        if (!element) {
          return std::nullopt;
        }

        values.push_back(*element);
      }
    }

    std::vector<std::byte> result(values.size() * sizeof(T));
    std::memcpy(result.data(), values.data(), result.size());

    return result;
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
        _defaultValue(),
        _size(0) {}

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
        _defaultValue(),
        _size(0) {
    if (_status != PropertyViewStatus::Valid) {
      return;
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue =
            getBooleanArrayValue(*classProperty.defaultProperty, _size);
      }

      if (_size == 0 || (_count > 0 && _size != _count)) {
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
        _count(0),
        _required(false),
        _defaultValue(),
        _size(0) {}

  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
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
    if (_size > 0) {
      return PropertyArrayView<bool>(
          std::span<const std::byte>(
              _defaultValue.data(),
              _defaultValue.size()),
          /* bitOffset = */ 0,
          _size);
    }

    return std::nullopt;
  }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;
  bool _required;

  std::vector<std::byte> _defaultValue;
  int64_t _size;

  static std::vector<std::byte> getBooleanArrayValue(
      const CesiumUtility::JsonValue& jsonValue,
      int64_t& size) {
    if (!jsonValue.isArray()) {
      return std::vector<std::byte>();
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    std::vector<std::byte> values;
    values.reserve((array.size() / 8) + 1);

    size_t byteIndex = 0;
    uint8_t bitIndex = 0;

    for (size_t i = 0; i < array.size(); i++, size++) {
      if (!array[i].isBool()) {
        size = 0;
        return values;
      }

      if (values.size() < byteIndex - 1) {
        values.push_back(std::byte(0));
      }

      std::byte value = std::byte(array[i].getBool() ? 1 : 0);
      value = value << bitIndex;

      values[byteIndex] |= value;

      bitIndex++;
      if (bitIndex > 7) {
        byteIndex++;
        bitIndex = 0;
      }
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
      if (!_required) {
        _noData = getStringArrayValue(*classProperty.noData);
      }

      if (_noData.size == 0 || (_count > 0 && _noData.size != _count)) {
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (classProperty.defaultProperty) {
      if (!_required) {
        _defaultValue = getStringArrayValue(*classProperty.defaultProperty);
      }

      if (_defaultValue.size == 0 ||
          (_count > 0 && _defaultValue.size != _count)) {
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
      const PropertyTableProperty& /*property*/)
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
    if (_noData.size > 0) {
      return PropertyArrayView<std::string_view>(
          std::span<const std::byte>(_noData.data.data(), _noData.data.size()),
          std::span<const std::byte>(
              _noData.offsets.data(),
              _noData.offsets.size()),
          _noData.offsetType,
          _noData.size);
    }

    return std::nullopt;
  }

  /**
   * @copydoc PropertyView<ElementType, false>::defaultValue
   */
  std::optional<PropertyArrayView<std::string_view>>
  defaultValue() const noexcept {
    if (_defaultValue.size > 0) {
      return PropertyArrayView<std::string_view>(
          std::span<const std::byte>(
              _defaultValue.data.data(),
              _defaultValue.data.size()),
          std::span<const std::byte>(
              _defaultValue.offsets.data(),
              _defaultValue.offsets.size()),
          _defaultValue.offsetType,
          _defaultValue.size);
    }

    return std::nullopt;
  }

protected:
  /** @copydoc PropertyViewStatus */
  PropertyViewStatusType _status;

private:
  std::optional<std::string> _name;
  std::optional<std::string> _semantic;
  std::optional<std::string> _description;

  int64_t _count;
  bool _required;

  struct StringArrayValue {
    std::vector<std::byte> data;
    std::vector<std::byte> offsets;
    PropertyComponentType offsetType = PropertyComponentType::None;
    int64_t size = 0;
  };

  StringArrayValue _noData;
  StringArrayValue _defaultValue;

  static StringArrayValue
  getStringArrayValue(const CesiumUtility::JsonValue& jsonValue) {
    StringArrayValue result;
    if (!jsonValue.isArray()) {
      return result;
    }

    std::vector<std::string> strings;
    std::vector<uint64_t> stringOffsets;

    const auto array = jsonValue.getArray();
    strings.reserve(array.size());
    stringOffsets.reserve(array.size() + 1);
    stringOffsets.push_back(static_cast<uint64_t>(0));

    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isString()) {
        // The entire array is invalidated; return.
        return result;
      }

      const std::string& string = array[i].getString();
      strings.push_back(string);
      stringOffsets.push_back(stringOffsets[i] + string.size());
    }

    uint64_t totalLength = stringOffsets.back();
    result.data.resize(totalLength);
    for (size_t i = 0; i < strings.size(); ++i) {
      std::memcpy(
          result.data.data() + stringOffsets[i],
          strings[i].data(),
          strings[i].size());
    };

    if (totalLength <= std::numeric_limits<uint8_t>::max()) {
      result.offsets = narrowOffsetsBuffer<uint8_t>(stringOffsets);
      result.offsetType = PropertyComponentType::Uint8;
    } else if (totalLength <= std::numeric_limits<uint16_t>::max()) {
      result.offsets = narrowOffsetsBuffer<uint16_t>(stringOffsets);
      result.offsetType = PropertyComponentType::Uint16;
    } else if (totalLength <= std::numeric_limits<uint32_t>::max()) {
      result.offsets = narrowOffsetsBuffer<uint32_t>(stringOffsets);
      result.offsetType = PropertyComponentType::Uint32;
    } else {
      result.offsets.resize(stringOffsets.size() * sizeof(uint64_t));
      std::memcpy(
          result.offsets.data(),
          stringOffsets.data(),
          result.offsets.size());
      result.offsetType = PropertyComponentType::Uint64;
    }

    result.size = static_cast<int64_t>(strings.size());

    return result;
  }

  template <typename T>
  static std::vector<std::byte>
  narrowOffsetsBuffer(std::vector<uint64_t> offsets) {
    std::vector<std::byte> result(offsets.size() * sizeof(T));
    size_t bufferOffset = 0;
    for (size_t i = 0; i < offsets.size(); i++, bufferOffset += sizeof(T)) {
      T offset = static_cast<T>(offsets[i]);
      std::memcpy(result.data() + bufferOffset, &offset, sizeof(T));
    }

    return result;
  }
};

} // namespace CesiumGltf
