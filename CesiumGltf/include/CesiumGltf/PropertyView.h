#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/PropertyTableProperty.h"
#include "CesiumGltf/PropertyTextureProperty.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <bitset>
#include <cstring>
#include <optional>

namespace CesiumGltf {

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
   * @brief This property view is trying to view a property that does not
   * exist.
   */
  static const PropertyViewStatusType ErrorNonexistentProperty = 1;

  /**
   * @brief This property view's type does not match what is
   * specified in {@link ClassProperty::type}.
   */
  static const PropertyViewStatusType ErrorTypeMismatch = 2;

  /**
   * @brief This property view's component type does not match what
   * is specified in {@link ClassProperty::componentType}.
   */
  static const PropertyViewStatusType ErrorComponentTypeMismatch = 3;

  /**
   * @brief This property view differs from what is specified in
   * {@link ClassProperty::array}.
   */
  static const PropertyViewStatusType ErrorArrayTypeMismatch = 4;

  /**
   * @brief This property says it is normalized, but is not of an integer
   * component type.
   */
  static const PropertyViewStatusType ErrorInvalidNormalization = 5;

  /**
   * @brief The property provided an invalid offset value.
   */
  static const PropertyViewStatusType ErrorInvalidOffset = 6;

  /**
   * @brief The property provided an invalid scale value.
   */
  static const PropertyViewStatusType ErrorInvalidScale = 7;

  /**
   * @brief The property provided an invalid maximum value.
   */
  static const PropertyViewStatusType ErrorInvalidMax = 8;

  /**
   * @brief The property provided an invalid minimum value.
   */
  static const PropertyViewStatusType ErrorInvalidMin = 9;

  /**
   * @brief The property provided an invalid "no data" value.
   */
  static const PropertyViewStatusType ErrorInvalidNoDataValue = 10;

  /**
   * @brief The property provided an invalid default value.
   */
  static const PropertyViewStatusType ErrorInvalidDefaultValue = 11;
};

template <typename ElementType, bool Normalized = false, typename Enable = void>
class PropertyView;

namespace {
template <typename T>
static std::optional<T> getScalar(const CesiumUtility::JsonValue& jsonValue) {
  try {
    return jsonValue.getSafeNumber<T>();
  } catch (const CesiumUtility::JsonValueNotRealValue& /*error*/) {
    return std::nullopt;
  } catch (const gsl::narrowing_error& /*error*/) {
    return std::nullopt;
  }
}

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

} // namespace

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
      : _status(PropertyViewStatus::Valid),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (convertPropertyTypeToString(TypeToPropertyType<ElementType>::value) !=
        classProperty.type) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.componentType &&
        TypeToPropertyType<ElementType>::component !=
            PropertyComponentType::None) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.componentType &&
        convertPropertyComponentTypeToString(
            TypeToPropertyType<ElementType>::component) !=
            *classProperty.componentType) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorInvalidNormalization;
    }

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (classProperty.offset) {
        _offset = getValue(*classProperty.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (classProperty.scale) {
        _scale = getValue(*classProperty.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (classProperty.max) {
        _max = getValue(*classProperty.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (classProperty.min) {
        _min = getValue(*classProperty.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }

    if (!_required) {
      if (classProperty.noData) {
        _noData = getValue(*classProperty.noData);
        if (!_noData) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidNoDataValue;
          return;
        }
      }

      if (classProperty.defaultProperty) {
        _defaultValue = getValue(*classProperty.defaultProperty);
        if (!_defaultValue) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidDefaultValue;
          return;
        }
      }
    }
  }

protected:
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (property.offset) {
        _offset = getValue(*property.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (property.scale) {
        _scale = getValue(*property.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (property.max) {
        _max = getValue(*property.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (property.min) {
        _min = getValue(*property.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (property.offset) {
        _offset = getValue(*property.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (property.scale) {
        _scale = getValue(*property.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (property.max) {
        _max = getValue(*property.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (property.min) {
        _min = getValue(*property.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }
  }

public:
  /**
   * @brief Gets the status of this property view, indicating whether an error
   * occurred.
   *
   * @return The status of this property view.
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @brief Get the element count of the fixed-length arrays in this property.
   * Only applicable when the property is an array type.
   *
   * @return The count of this property.
   */
  virtual int64_t arrayCount() const noexcept { return 0; }

  /**
   * @brief Whether this property has a normalized integer type.
   *
   * @return Whether this property has a normalized integer type.
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @brief Gets the offset to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's offset, or std::nullopt if it was not specified.
   */
  virtual std::optional<ElementType> offset() const noexcept { return _offset; }

  /**
   * @brief Gets the scale to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's scale, or std::nullopt if it was not specified.
   */
  virtual std::optional<ElementType> scale() const noexcept { return _scale; }

  /**
   * @brief Gets the maximum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the maximum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's maximum value, or std::nullopt if it was not
   * specified.
   */
  virtual std::optional<ElementType> max() const noexcept { return _max; }

  /**
   * @brief Gets the minimum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the minimum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's minimum value, or std::nullopt if it was not
   * specified.
   */
  virtual std::optional<ElementType> min() const noexcept { return _min; }

  /**
   * @brief Whether the property must be present in every entity conforming to
   * the class. If not required, instances of the property may include "no data"
   * values, or the entire property may be omitted.
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @brief Gets the "no data" value, i.e., the value representing missing data
   * in the property wherever it appears. Also known as a sentinel value. This
   * is given as the plain property value, without the transforms from the
   * normalized, offset, and scale properties.
   */
  virtual std::optional<ElementType> noData() const noexcept { return _noData; }

  /**
   * @brief Gets the default value to use when encountering a "no data" value or
   * an omitted property. The value is given in its final form, taking the
   * effect of normalized, offset, and scale properties into account.
   */
  virtual std::optional<ElementType> defaultValue() const noexcept {
    return _defaultValue;
  }

private:
  PropertyViewStatusType _status;
  bool _required;

  std::optional<ElementType> _offset;
  std::optional<ElementType> _scale;
  std::optional<ElementType> _max;
  std::optional<ElementType> _min;

  std::optional<ElementType> _noData;
  std::optional<ElementType> _defaultValue;

  /**
   * @brief Attempts to parse from the given json value.
   *
   * If T is a type with multiple components, e.g. a VECN or MATN type, this
   * will return std::nullopt if one or more components could not be parsed.
   *
   * @return The value as an instance of T, or std::nullopt if it could not be
   * parsed.
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
 * @tparam ElementType The C++ type of the values in this property
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
      : _status(PropertyViewStatus::Valid),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (convertPropertyTypeToString(TypeToPropertyType<ElementType>::value) !=
        classProperty.type) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.componentType &&
        TypeToPropertyType<ElementType>::component !=
            PropertyComponentType::None) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.componentType &&
        convertPropertyComponentTypeToString(
            TypeToPropertyType<ElementType>::component) !=
            *classProperty.componentType) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!classProperty.normalized) {
      _status = PropertyViewStatus::ErrorInvalidNormalization;
    }

    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (classProperty.offset) {
        _offset = getValue<NormalizedType>(*classProperty.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (classProperty.scale) {
        _scale = getValue<NormalizedType>(*classProperty.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (classProperty.max) {
        _max = getValue<NormalizedType>(*classProperty.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (classProperty.min) {
        _min = getValue<NormalizedType>(*classProperty.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }

    if (!_required) {
      if (classProperty.noData) {
        _noData = getValue<ElementType>(*classProperty.noData);
        if (!_noData) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidNoDataValue;
          return;
        }
      }

      if (classProperty.defaultProperty) {
        _defaultValue =
            getValue<NormalizedType>(*classProperty.defaultProperty);
        if (!_defaultValue) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidDefaultValue;
          return;
        }
      }
    }
  }

protected:
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
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
    }
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
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
    }
  }

public:
  /**
   * @brief Gets the status of this property view, indicating whether an error
   * occurred.
   *
   * @return The status of this property view.
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<NormalizedType> offset() const noexcept {
    return _offset;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<NormalizedType> scale() const noexcept {
    return _scale;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<NormalizedType> max() const noexcept { return _max; }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<NormalizedType> min() const noexcept { return _min; }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<ElementType> noData() const noexcept { return _noData; }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<NormalizedType> defaultValue() const noexcept {
    return _defaultValue;
  }

private:
  PropertyViewStatusType _status;
  bool _required;

  std::optional<NormalizedType> _offset;
  std::optional<NormalizedType> _scale;
  std::optional<NormalizedType> _max;
  std::optional<NormalizedType> _min;

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
};

template <> class PropertyView<bool> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _required(false),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(PropertyViewStatus::Valid),
        _required(classProperty.required),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::BOOLEAN) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required && classProperty.defaultProperty) {
      _defaultValue = getBooleanValue(*classProperty.defaultProperty);
      if (!_defaultValue) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
      : PropertyView(classProperty) {}

  /**
   * @brief Constructs a property instance from a property texture property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& /*property*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc IPropertyView::status
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<bool> offset() const noexcept { return std::nullopt; }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<bool> scale() const noexcept { return std::nullopt; }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<bool> max() const noexcept { return std::nullopt; }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<bool> min() const noexcept { return std::nullopt; }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<bool> noData() const noexcept { return std::nullopt; }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<bool> defaultValue() const noexcept {
    return _defaultValue;
  }

private:
  PropertyViewStatusType _status;
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

template <> class PropertyView<std::string_view> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(PropertyViewStatus::Valid),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::STRING) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required) {
      if (classProperty.noData) {
        _noData = getStringValue(*classProperty.noData);
        if (!_noData) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidNoDataValue;
          return;
        }
      }
      if (classProperty.defaultProperty) {
        _defaultValue = getStringValue(*classProperty.defaultProperty);
        if (!_defaultValue) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidDefaultValue;
          return;
        }
      }
    }
  }

protected:
  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
      : PropertyView(classProperty) {}

  /**
   * @brief Constructs a property instance from a property texture property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& /*property*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc IPropertyView::status
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<std::string_view> offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<std::string_view> scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<std::string_view> max() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<std::string_view> min() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<std::string_view> noData() const noexcept {
    if (_noData)
      return std::string_view(*_noData);

    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<std::string_view> defaultValue() const noexcept {
    if (_defaultValue)
      return std::string_view(*_defaultValue);

    return std::nullopt;
  }

private:
  PropertyViewStatusType _status;
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

template <typename ElementType>
class PropertyView<PropertyArrayView<ElementType>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _count(0),
        _normalized(false),
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
      : _status(PropertyViewStatus::Valid),
        _count(_count = classProperty.count ? *classProperty.count : 0),
        _normalized(classProperty.normalized),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (convertPropertyTypeToString(TypeToPropertyType<ElementType>::value) !=
        classProperty.type) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.componentType &&
        TypeToPropertyType<ElementType>::component !=
            PropertyComponentType::None) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.componentType &&
        convertPropertyComponentTypeToString(
            TypeToPropertyType<ElementType>::component) !=
            *classProperty.componentType) {
      _status = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (classProperty.normalized) {
      _status = PropertyViewStatus::ErrorInvalidNormalization;
      return;
    }

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (classProperty.offset) {
        _offset = getArrayValue(*classProperty.offset);
        if (!_offset ||
            (_count > 0 && _offset->size() != static_cast<size_t>(_count))) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (classProperty.scale) {
        _scale = getArrayValue(*classProperty.scale);
        if (!_scale ||
            (_count > 0 && _scale->size() != static_cast<size_t>(_count))) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (classProperty.max) {
        _max = getArrayValue(*classProperty.max);
        if (!_max ||
            (_count > 0 && _max->size() != static_cast<size_t>(_count))) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (classProperty.min) {
        _min = getArrayValue(*classProperty.min);
        if (!_min ||
            (_count > 0 && _min->size() != static_cast<size_t>(_count))) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }

    if (!_required) {
      if (classProperty.noData) {
        _noData = getArrayValue(*classProperty.noData);
        if (!_noData) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidNoDataValue;
          return;
        }
      }

      if (classProperty.defaultProperty) {
        _defaultValue = getArrayValue(*classProperty.defaultProperty);
        if (!_defaultValue) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidDefaultValue;
          return;
        }
      }
    }
  }

protected:
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (property.offset) {
        _offset = getArrayValue(*property.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (property.scale) {
        _scale = getArrayValue(*property.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (property.max) {
        _max = getArrayValue(*property.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (property.min) {
        _min = getArrayValue(*property.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }
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

    // If the property has its own values,  the class-provided values.
    if constexpr (IsMetadataNumeric<ElementType>::value) {
      if (property.offset) {
        _offset = getArrayValue(*property.offset);
        if (!_offset) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidOffset;
          return;
        }
      }

      if (property.scale) {
        _scale = getArrayValue(*property.scale);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidScale;
          return;
        }
      }

      if (property.max) {
        _max = getArrayValue(*property.max);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMax;
          return;
        }
      }

      if (property.min) {
        _min = getArrayValue(*property.min);
        if (!_scale) {
          // The value was specified but something went wrong.
          _status = PropertyViewStatus::ErrorInvalidMin;
          return;
        }
      }
    }
  }

public:
  /**
   * @copydoc IPropertyView::status
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return _normalized; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  offset() const noexcept {
    if (!_offset) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        gsl::span<const std::byte>(_offset->data(), _offset->size()));
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<ElementType>> scale() const noexcept {
    if (!_scale) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        gsl::span<const std::byte>(_scale->data(), _scale->size()));
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<ElementType>> max() const noexcept {
    if (!_max) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        gsl::span<const std::byte>(_max->data(), _max->size()));
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<ElementType>> min() const noexcept {
    if (!_min) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        gsl::span<const std::byte>(_min->data(), _min->size()));
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  noData() const noexcept {
    if (!_noData) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(
        gsl::span<const std::byte>(_noData->data(), _noData->size()));
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  defaultValue() const noexcept {
    if (!_defaultValue) {
      return std::nullopt;
    }

    return PropertyArrayView<ElementType>(gsl::span<const std::byte>(
        _defaultValue->data(),
        _defaultValue->size()));
  }

private:
  PropertyViewStatusType _status;
  int64_t _count;
  bool _normalized;

  std::optional<std::vector<std::byte>> _offset;
  std::optional<std::vector<std::byte>> _scale;
  std::optional<std::vector<std::byte>> _max;
  std::optional<std::vector<std::byte>> _min;

  bool _required;
  std::optional<std::vector<std::byte>> _noData;
  std::optional<std::vector<std::byte>> _defaultValue;

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

  template <typename T>
  static std::optional<T> getScalar(const CesiumUtility::JsonValue& jsonValue) {
    try {
      return jsonValue.getSafeNumber<T>();
    } catch (const CesiumUtility::JsonValueNotRealValue& /*error*/) {
      return std::nullopt;
    } catch (const gsl::narrowing_error& /*error*/) {
      return std::nullopt;
    }
  }

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
};

template <> class PropertyView<PropertyArrayView<bool>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _count(0),
        _required(false),
        _defaultValue(),
        _size(0) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(PropertyViewStatus::Valid),
        _count(classProperty.count ? *classProperty.count : 0),
        _required(classProperty.required),
        _defaultValue(),
        _size(0) {
    if (classProperty.type != ClassProperty::Type::BOOLEAN) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required && classProperty.defaultProperty) {
      _defaultValue =
          getBooleanArrayValue(*classProperty.defaultProperty, _size);
      if (_size == 0 || (_count > 0 && _size != _count)) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
      : PropertyView(classProperty) {}

  /**
   * @brief Constructs a property instance from a property texture property
   * and its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& /*property*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc IPropertyView::status
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<bool>> offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<bool>> scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<bool>> max() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<bool>> min() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<bool>> noData() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<bool>> defaultValue() const noexcept {
    if (_size > 0) {
      return PropertyArrayView<bool>(
          gsl::span<const std::byte>(
              _defaultValue.data(),
              _defaultValue.size()),
          /* bitOffset = */ 0,
          _size);
    }

    return std::nullopt;
  }

private:
  PropertyViewStatusType _status;
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

template <> class PropertyView<PropertyArrayView<std::string_view>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _status(PropertyViewStatus::ErrorNonexistentProperty),
        _count(0),
        _required(false),
        _noData(),
        _noDataOffsets(),
        _noDataOffsetType(PropertyComponentType::None),
        _noDataSize(0),
        _defaultValue(),
        _defaultValueOffsets(),
        _defaultValueOffsetType(PropertyComponentType::None),
        _defaultValueSize(0) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _status(PropertyViewStatus::Valid),
        _count(classProperty.count ? *classProperty.count : 0),
        _required(classProperty.required),
        _noData(),
        _noDataOffsets(),
        _noDataOffsetType(PropertyComponentType::None),
        _noDataSize(0),
        _defaultValue(),
        _defaultValueOffsets(),
        _defaultValueOffsetType(PropertyComponentType::None),
        _defaultValueSize(0) {
    if (classProperty.type != ClassProperty::Type::STRING) {
      _status = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _status = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required && classProperty.noData) {
      _noData = getStringArrayValue(
          *classProperty.noData,
          _noDataOffsets,
          _noDataOffsetType,
          _noDataSize);
      if (_noDataSize == 0 || (_count > 0 && _noDataSize != _count)) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidNoDataValue;
        return;
      }
    }

    if (!_required && classProperty.defaultProperty) {
      _defaultValue = getStringArrayValue(
          *classProperty.defaultProperty,
          _defaultValueOffsets,
          _defaultValueOffsetType,
          _defaultValueSize);
      if (_defaultValueSize == 0 || (_count > 0 && _noDataSize != _count)) {
        // The value was specified but something went wrong.
        _status = PropertyViewStatus::ErrorInvalidDefaultValue;
        return;
      }
    }
  }

protected:
  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
      : PropertyView(classProperty) {}

  /**
   * @brief Constructs a property instance from a property texture property
   * and its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& /*property*/)
      : PropertyView(classProperty) {}

public:
  /**
   * @copydoc IPropertyView::status
   */
  virtual PropertyViewStatusType status() const noexcept { return _status; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  offset() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  scale() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  max() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  min() const noexcept {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  noData() const noexcept {
    if (_noDataSize > 0) {
      return PropertyArrayView<std::string_view>(
          gsl::span<const std::byte>(_noData.data(), _noData.size()),
          gsl::span<const std::byte>(
              _noDataOffsets.data(),
              _noDataOffsets.size()),
          _noDataOffsetType,
          _noDataSize);
    }

    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  defaultValue() const noexcept {
    if (_noDataSize > 0) {
      return PropertyArrayView<std::string_view>(
          gsl::span<const std::byte>(
              _defaultValue.data(),
              _defaultValue.size()),
          gsl::span<const std::byte>(
              _defaultValueOffsets.data(),
              _defaultValueOffsets.size()),
          _defaultValueOffsetType,
          _defaultValueSize);
    }

    return std::nullopt;
  }

private:
  PropertyViewStatusType _status;
  int64_t _count;
  bool _required;

  std::vector<std::byte> _noData;
  std::vector<std::byte> _noDataOffsets;
  PropertyComponentType _noDataOffsetType;
  int64_t _noDataSize;

  std::vector<std::byte> _defaultValue;
  std::vector<std::byte> _defaultValueOffsets;
  PropertyComponentType _defaultValueOffsetType;
  int64_t _defaultValueSize;

  static std::vector<std::byte> getStringArrayValue(
      const CesiumUtility::JsonValue& jsonValue,
      std::vector<std::byte>& offsets,
      PropertyComponentType& offsetType,
      int64_t& size) {
    if (!jsonValue.isArray()) {
      return std::vector<std::byte>();
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
        return std::vector<std::byte>();
      }

      const std::string& string = array[i].getString();
      strings.push_back(string);
      stringOffsets.push_back(stringOffsets[i] + string.size());
    }

    uint64_t totalLength = stringOffsets.back();
    std::vector<std::byte> values(totalLength);
    for (size_t i = 0; i < strings.size(); ++i) {
      std::memcpy(
          values.data() + stringOffsets[i],
          strings[i].data(),
          strings[i].size());
    };

    if (totalLength <= std::numeric_limits<uint8_t>::max()) {
      offsets = narrowOffsetsBuffer<uint8_t>(stringOffsets);
      offsetType = PropertyComponentType::Uint8;
    } else if (totalLength <= std::numeric_limits<uint16_t>::max()) {
      offsets = narrowOffsetsBuffer<uint16_t>(stringOffsets);
      offsetType = PropertyComponentType::Uint16;
    } else if (totalLength <= std::numeric_limits<uint32_t>::max()) {
      offsets = narrowOffsetsBuffer<uint32_t>(stringOffsets);
      offsetType = PropertyComponentType::Uint32;
    } else {
      offsets.resize(stringOffsets.size() * sizeof(uint64_t));
      std::memcpy(offsets.data(), stringOffsets.data(), offsets.size());
      offsetType = PropertyComponentType::Uint64;
    }

    size = static_cast<int64_t>(strings.size());

    return values;
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
