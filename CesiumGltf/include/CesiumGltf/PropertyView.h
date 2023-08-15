#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/PropertyTableProperty.h"
#include "CesiumGltf/PropertyTextureProperty.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <bitset>
#include <optional>

namespace CesiumGltf {
/**
 * @brief Indicates the status of a property view.
 *
 * The {@link PropertyView} constructor always completes successfully.
 * However, it may find fundamental errors within the property definition
 * itself. This enumeration provides the reason.
 */
enum class PropertyViewStatus {
  /**
   * @brief This property view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This property view is trying to view a property that does not exist.
   */
  ErrorNonexistentProperty,

  /**
   * @brief This property view's type does not match what is
   * specified in {@link ClassProperty::type}.
   */
  ErrorTypeMismatch,

  /**
   * @brief This property view's component type does not match what
   * is specified in {@link ClassProperty::componentType}.
   */
  ErrorComponentTypeMismatch,

  /**
   * @brief This property view differs from what is specified in
   * {@link ClassProperty::array}.
   */
  ErrorArrayTypeMismatch,
};

/**
 * @brief An interface for generic metadata property in EXT_structural_metadata.
 *
 * Whether they belong to property tables, property textures, or property
 * attributes, properties have their own sub-properties affecting the actual
 * property values. Although they are typically defined via class property, they
 * may be overriden by individual instances of the property. The constructor is
 * responsible for resolving those differences.
 */
template <typename ElementType> class IPropertyView {
public:
  /**
   * @brief Get the element count of the fixed-length arrays in this property.
   * Only applicable when the property is an array type.
   *
   * @return The count of this property.
   */
  virtual int64_t arrayCount() const noexcept = 0;

  /**
   * @brief Whether this property has a normalized integer type.
   *
   * @return Whether this property has a normalized integer type.
   */
  virtual bool normalized() const noexcept = 0;

  /**
   * @brief Gets the offset to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's offset, or std::nullopt if it was not specified.
   */
  virtual std::optional<ElementType> offset() const noexcept = 0;
  /**
   * @brief Gets the scale to apply to property values. Only applicable to
   * SCALAR, VECN, and MATN types when the component type is FLOAT32 or
   * FLOAT64, or when the property is normalized.
   *
   * @returns The property's scale, or std::nullopt if it was not specified.
   */
  virtual std::optional<ElementType> scale() const noexcept = 0;

  /**
   * @brief Gets the maximum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the maximum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's maximum value, or std::nullopt if it was not
   * specified.
   */
  virtual std::optional<ElementType> max() const noexcept = 0;

  /**
   * @brief Gets the minimum allowed value for the property. Only applicable to
   * SCALAR, VECN, and MATN types. This is the minimum of all property
   * values, after the transforms based on the normalized, offset, and
   * scale properties have been applied.
   *
   * @returns The property's minimum value, or std::nullopt if it was not
   * specified.
   */
  virtual std::optional<ElementType> min() const noexcept = 0;

  /**
   * @brief Whether the property must be present in every entity conforming to
   * the class. If not required, instances of the property may include "no data"
   * values, or the entire property may be omitted.
   */
  virtual bool required() const noexcept = 0;

  /**
   * @brief Gets the "no data" value, i.e., the value representing missing data
   * in the property wherever it appears. Also known as a sentinel value. This
   * is given as the plain property value, without the transforms from the
   * normalized, offset, and scale properties.
   */
  virtual std::optional<ElementType> noData() const noexcept = 0;

  /**
   * @brief Gets the default value to use when encountering a "no data" value or
   * an omitted property. The value is given in its final form, taking the
   * effect of normalized, offset, and scale properties into account.
   */
  virtual std::optional<ElementType> defaultValue() const noexcept = 0;

protected:
  /**
   * @brief Gets the status of the property view. Derived classes will often
   * have their own status enumerations, so this is only used internally.
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept = 0;
};

/**
 * @brief Represents a generic metadata property in EXT_structural_metadata.
 * Implements the {@link IPropertyView} interface.
 *
 * Child classes should validate that the class property type matches the
 * "ElementType" before constructing a property view.
 */
template <typename ElementType>
class PropertyView : IPropertyView<ElementType> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
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
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _normalized(classProperty.normalized),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (convertStringToPropertyType(classProperty.type) !=
        TypeToPropertyType<ElementType>::value) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.componentType &&
        TypeToPropertyType<ElementType>::component !=
            PropertyComponentType::None) {
      _propertyViewStatus = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.componentType &&
        convertStringToPropertyComponentType(*classProperty.componentType) !=
            TypeToPropertyType<ElementType>::component) {
      _propertyViewStatus = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    _offset = classProperty.offset
                  ? getValue<ElementType>(*classProperty.offset)
                  : std::nullopt;
    _scale = classProperty.scale ? getValue<ElementType>(*classProperty.scale)
                                 : std::nullopt;
    _max = classProperty.max ? getValue<ElementType>(*classProperty.max)
                             : std::nullopt;
    _min = classProperty.min ? getValue<ElementType>(*classProperty.min)
                             : std::nullopt;

    if (!_required) {
      _noData = classProperty.noData
                    ? getValue<ElementType>(*classProperty.noData)
                    : std::nullopt;
      _defaultValue =
          classProperty.defaultProperty
              ? getValue<ElementType>(*classProperty.defaultProperty)
              : std::nullopt;
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
    if (_propertyViewStatus != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (property.offset) {
      _offset = getValue<ElementType>(*property.offset);
    }

    if (property.scale) {
      _scale = getValue<ElementType>(*property.scale);
    }

    if (property.max) {
      _max = getValue<ElementType>(*property.max);
    }

    if (property.min) {
      _min = getValue<ElementType>(*property.min);
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
    if (_propertyViewStatus != PropertyViewStatus::Valid) {
      return;
    }

    // If the property has its own values, override the class-provided values.
    if (property.offset) {
      _offset = getValue<ElementType>(*property.offset);
    }

    if (property.scale) {
      _scale = getValue<ElementType>(*property.scale);
    }

    if (property.max) {
      _max = getValue<ElementType>(*property.max);
    }

    if (property.min) {
      _min = getValue<ElementType>(*property.min);
    }
  }

public:
  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return _normalized; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<ElementType> offset() const noexcept override {
    return _offset;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<ElementType> scale() const noexcept override {
    return _scale;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<ElementType> max() const noexcept override {
    return _max;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<ElementType> min() const noexcept override {
    return _min;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual std::optional<ElementType> noData() const noexcept override {
    return _noData;
  }

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual std::optional<ElementType> defaultValue() const noexcept override {
    return _defaultValue;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;

  bool _normalized;

  std::optional<ElementType> _offset;
  std::optional<ElementType> _scale;
  std::optional<ElementType> _max;
  std::optional<ElementType> _min;

  bool _required;
  std::optional<ElementType> _noData;
  std::optional<ElementType> _defaultValue;

  /**
   * @brief Attempts to parse from the given json value.
   *
   * If T is a type with multiple components, e.g. a VECN or MATN type, this
   * will return std::nullopt if one or more components could not be parsed.
   *
   * If T is an array view, this will return std::nullopt if one or
   * more entries could not be parsed.
   *
   * @return The value as an instance of T, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static std::optional<T> getValue(const CesiumUtility::JsonValue& jsonValue) {
    if constexpr (IsMetadataScalar<T>::value) {
      return getScalar<T>(jsonValue);
    } else

        if constexpr (IsMetadataVecN<T>::value) {
      return getVecN<T>(jsonValue);
    } else
      return std::nullopt;

    // if constexpr (IsMetadataMatN<T>::value) {
    //}
  }

  /**
   * @brief Attempts to parse from the given json value. If it could not be
   * parsed as an ElementType, this returns the default value.
   *
   * If ElementType is a type with multiple components, e.g. a VECN or MATN
   * type.
   *
   * @return The value as an ElementType, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static T getValueOrDefault(
      const CesiumUtility::JsonValue& jsonValue,
      const T& defaultValue) {
    if constexpr (IsMetadataScalar<T>::value) {
      return jsonValue.getSafeNumberOrDefault<T>(defaultValue);
    }

    if constexpr (IsMetadataVecN<T>::value) {
      return getVecNOrDefault(jsonValue, defaultValue);
    }

    if constexpr (IsMetadataMatN<T>::value) {
    }
  }

  /**
   * @brief Attempts to parse from the given json value. If it could not be
   * parsed as an ElementType, this returns the default value.
   *
   * If ElementType is a type with multiple components, e.g. a VECN or MATN
   * type.
   *
   * @return The value as an ElementType, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static std::optional<T> getScalar(const CesiumUtility::JsonValue& jsonValue) {
    try {
      return jsonValue.getSafeNumber<ElementType>();
    } catch (const CesiumUtility::JsonValueNotRealValue& /*error*/) {
      return std::nullopt;
    } catch (const gsl::narrowing_error& /*error*/) {
      // Continue below.
    }

    // If the value is in range, but it loses precision, a narrowing_error will
    // be returned. Try to round to a reasonable number anyways, even if it
    // loses precision.
    if (jsonValue.isInt64()) {
      int64_t int64Value = jsonValue.getInt64();
      if (int64Value >=
              static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
          int64Value <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(int64Value);
      }
    } else if (jsonValue.isUint64()) {
      uint64_t uint64Value = jsonValue.getUint64();
      if (uint64Value <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(uint64Value);
      }
    } else if (jsonValue.isDouble()) {
      double doubleValue = jsonValue.getDouble();
      if (doubleValue >=
              static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
          doubleValue <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(doubleValue);
      }
    }

    return std::nullopt;
  }

  template <typename VecType>
  static std::optional<VecType>
  getVecN(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return std::nullopt;
    }

    return std::nullopt;
    // const CesiumUtility::JsonValue::Array& array = value.getArray();
    // glm::length_t N = VecType::length();
    // if (array.size() != N) {
    //  return std::nullopt;
    //}

    // using T = VecType::type;

    // glm::length<N, T> result;
    // for (glm::length_t i = 0; i < N; i++) {
    //  std::optional<T> value = getValue<T>(array[i]);
    //  if (!value) {
    //    return std::nullopt;
    //  }

    //  result[i] = value;
    //}

    // return result;
  }

  template <typename VecType>
  static VecType getVecNOrDefault(
      const CesiumUtility::JsonValue& value,
      const VecType& defaultValue) {
    if (!jsonValue.isArray()) {
      return defaultValue;
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    glm::length_t N = VecType::length();
    if (array.size() != N) {
      return defaultValue;
    }

    using T = VecType::type;

    VecType result;
    for (glm::length_t i = 0; i < N; i++) {
      std::optional<T> value = getValue(array[i]);
      if (!value) {
        return std::nullopt;
      }

      result[i] = value ? *value : defaultValue;
    }

    return result;
  }

  // template <typename MatType>
  // static std::optional<MatType>
  // getMatNFromJsonValue(const CesiumUtility::JsonValue& value) {
  //  if (!jsonValue.isArray()) {
  //    return std::nullopt;
  //  }

  //  const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
  //  if (array.size() != N) {
  //    return std::nullopt;
  //  }

  //  glm::vec<N, T, Q> result;
  //  for (glm::length_t i = 0; i < N; i++) {
  //    std::optional<T> value = getValue(array[i]);
  //    if (!value) {
  //      return std::nullopt;
  //    }

  //    result[i] = value;
  //  }

  //  return result;
  //}
};

template <> class PropertyView<bool> : IPropertyView<bool> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
        _required(false),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _required(classProperty.required),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::BOOLEAN) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required) {
      _defaultValue = classProperty.defaultProperty
                          ? getBooleanValue(*classProperty.defaultProperty)
                          : std::nullopt;
    }
  }

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

  ~PropertyView() {}

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<bool> offset() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<bool> scale() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<bool> max() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<bool> min() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<bool> noData() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<bool> defaultValue() const noexcept override {
    return _defaultValue;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;
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

template <>
class PropertyView<std::string_view> : IPropertyView<std::string_view> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::STRING) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    if (!_required) {
      _noData = classProperty.noData ? getStringValue(*classProperty.noData)
                                     : std::nullopt;
      _defaultValue = classProperty.defaultProperty
                          ? getStringValue(*classProperty.defaultProperty)
                          : std::nullopt;
    }
  }

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

  ~PropertyView() {}

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return 0; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<std::string_view> offset() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<std::string_view> scale() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<std::string_view> max() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<std::string_view> min() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<std::string_view> noData() const noexcept override {
    if (_noData)
      return std::string_view(*_noData);

    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<std::string_view>
  defaultValue() const noexcept override {
    if (_defaultValue)
      return std::string_view(*_defaultValue);

    return std::nullopt;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;
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
class PropertyView<PropertyArrayView<ElementType>>
    : IPropertyView<PropertyArrayView<ElementType>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
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
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _count(0),
        _normalized(classProperty.normalized),
        _offset(std::nullopt),
        _scale(std::nullopt),
        _max(std::nullopt),
        _min(std::nullopt),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (convertStringToPropertyType(classProperty.type) !=
        TypeToPropertyType<ElementType>::value) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.componentType &&
        TypeToPropertyType<ElementType>::component !=
            PropertyComponentType::None) {
      _propertyViewStatus = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (classProperty.componentType &&
        convertStringToPropertyComponentType(*classProperty.componentType) !=
            TypeToPropertyType<ElementType>::component) {
      _propertyViewStatus = PropertyViewStatus::ErrorComponentTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    _count = classProperty.count ? *classProperty.count : 0;

    //_offset = classProperty.offset
    //              ? getValue<ElementType>(*classProperty.offset)
    //              : std::nullopt;
    //_scale = classProperty.scale ? getValue<ElementType>(*classProperty.scale)
    //                             : std::nullopt;
    //_max = classProperty.max ? getValue<ElementType>(*classProperty.max)
    //                         : std::nullopt;
    //_min = classProperty.min ? getValue<ElementType>(*classProperty.min)
    //                         : std::nullopt;

    //if (!_required) {
    //  _noData = classProperty.noData
    //                ? getValue<ElementType>(*classProperty.noData)
    //                : std::nullopt;
    //  _defaultValue =
    //      classProperty.defaultProperty
    //          ? getValue<ElementType>(*classProperty.defaultProperty)
    //          : std::nullopt;
    //}
  }

protected:
  /**
   * @brief Constructs a property instance from a property table property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTableProperty& /*property*/)
      : PropertyView(classProperty) {
    if (_propertyViewStatus != PropertyViewStatus::Valid) {
      return;
    }

    //// If the property has its own values, override the class-provided values.
    //if (property.offset) {
    //  _offset = getValue<ElementType>(*property.offset);
    //}

    //if (property.scale) {
    //  _scale = getValue<ElementType>(*property.scale);
    //}

    //if (property.max) {
    //  _max = getValue<ElementType>(*property.max);
    //}

    //if (property.min) {
    //  _min = getValue<ElementType>(*property.min);
    //}
  }

  /**
   * @brief Constructs a property instance from a property texture property and
   * its class definition.
   */
  PropertyView(
      const ClassProperty& classProperty,
      const PropertyTextureProperty& /*property*/)
      : PropertyView(classProperty) {
    if (_propertyViewStatus != PropertyViewStatus::Valid) {
      return;
    }

    //// If the property has its own values, override the class-provided values.
    //if (property.offset) {
    //  _offset = getValue<ElementType>(*property.offset);
    //}

    //if (property.scale) {
    //  _scale = getValue<ElementType>(*property.scale);
    //}

    //if (property.max) {
    //  _max = getValue<ElementType>(*property.max);
    //}

    //if (property.min) {
    //  _min = getValue<ElementType>(*property.min);
    //}
  }

public:
  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return _normalized; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  offset() const noexcept override {
    return _offset;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  scale() const noexcept override {
    return _scale;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  max() const noexcept override {
    return _max;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  min() const noexcept override {
    return _min;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  noData() const noexcept override {
    return _noData;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<ElementType>>
  defaultValue() const noexcept override {
    return _defaultValue;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;

  int64_t _count;
  bool _normalized;

  std::optional<PropertyArrayView<ElementType>> _offset;
  std::optional<PropertyArrayView<ElementType>> _scale;
  std::optional<PropertyArrayView<ElementType>> _max;
  std::optional<PropertyArrayView<ElementType>> _min;

  bool _required;
  std::optional<PropertyArrayView<ElementType>> _noData;
  std::optional<PropertyArrayView<ElementType>> _defaultValue;

  /**
   * @brief Attempts to parse from the given json value.
   *
   * If T is a type with multiple components, e.g. a VECN or MATN type, this
   * will return std::nullopt if one or more components could not be parsed.
   *
   * If T is an array view, this will return std::nullopt if one or
   * more entries could not be parsed.
   *
   * @return The value as an instance of T, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static std::optional<T> getValue(const CesiumUtility::JsonValue& jsonValue) {
    if constexpr (IsMetadataScalar<T>::value) {
      return getScalar<T>(jsonValue);
    } else

        if constexpr (IsMetadataVecN<T>::value) {
      return getVecN<T>(jsonValue);
    } else
      return std::nullopt;

    // if constexpr (IsMetadataMatN<T>::value) {
    //}
  }

  /**
   * @brief Attempts to parse from the given json value. If it could not be
   * parsed as an ElementType, this returns the default value.
   *
   * If ElementType is a type with multiple components, e.g. a VECN or MATN
   * type.
   *
   * @return The value as an ElementType, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static T getValueOrDefault(
      const CesiumUtility::JsonValue& jsonValue,
      const T& defaultValue) {
    if constexpr (IsMetadataScalar<T>::value) {
      return jsonValue.getSafeNumberOrDefault<T>(defaultValue);
    }

    if constexpr (IsMetadataVecN<T>::value) {
      return getVecNOrDefault(jsonValue, defaultValue);
    }

    if constexpr (IsMetadataMatN<T>::value) {
    }

    if constexpr (IsMetadataArray<T>::value) {
      return defaultValue;
      //  if (!jsonValue.isArray()) {
      //    return PropertyArrayView<ElementType>(std::vector<ElementType>());
      //  }

      //  return jsonValue.isArray()
      //             ? getArrayValueOrDefault(jsonValue.getArray(),
      //             defaultValue) : defaultValue;
    }
  }

  /**
   * @brief Attempts to parse from the given json value. If it could not be
   * parsed as an ElementType, this returns the default value.
   *
   * If ElementType is a type with multiple components, e.g. a VECN or MATN
   * type.
   *
   * @return The value as an ElementType, or std::nullopt if it could not be
   * parsed.
   */
  template <typename T>
  static std::optional<T> getScalar(const CesiumUtility::JsonValue& jsonValue) {
    try {
      return jsonValue.getSafeNumber<ElementType>();
    } catch (const CesiumUtility::JsonValueNotRealValue& /*error*/) {
      return std::nullopt;
    } catch (const gsl::narrowing_error& /*error*/) {
      // Continue below.
    }

    // If the value is in range, but it loses precision, a narrowing_error will
    // be returned. Try to round to a reasonable number anyways, even if it
    // loses precision.
    if (jsonValue.isInt64()) {
      int64_t int64Value = jsonValue.getInt64();
      if (int64Value >=
              static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
          int64Value <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(int64Value);
      }
    } else if (jsonValue.isUint64()) {
      uint64_t uint64Value = jsonValue.getUint64();
      if (uint64Value <= static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(uint64Value);
      }
    } else if (jsonValue.isDouble()) {
      double doubleValue = jsonValue.getDouble();
      if (doubleValue >=
              static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
          doubleValue <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        return static_cast<T>(doubleValue);
      }
    }

    return std::nullopt;
  }

  template <typename VecType>
  static std::optional<VecType>
  getVecN(const CesiumUtility::JsonValue& jsonValue) {
    if (!jsonValue.isArray()) {
      return std::nullopt;
    }

    return std::nullopt;
    // const CesiumUtility::JsonValue::Array& array = value.getArray();
    // glm::length_t N = VecType::length();
    // if (array.size() != N) {
    //  return std::nullopt;
    //}

    // using T = VecType::type;

    // glm::length<N, T> result;
    // for (glm::length_t i = 0; i < N; i++) {
    //  std::optional<T> value = getValue<T>(array[i]);
    //  if (!value) {
    //    return std::nullopt;
    //  }

    //  result[i] = value;
    //}

    // return result;
  }

  template <typename VecType>
  static VecType getVecNOrDefault(
      const CesiumUtility::JsonValue& value,
      const VecType& defaultValue) {
    if (!jsonValue.isArray()) {
      return defaultValue;
    }

    const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
    glm::length_t N = VecType::length();
    if (array.size() != N) {
      return defaultValue;
    }

    using T = VecType::type;

    VecType result;
    for (glm::length_t i = 0; i < N; i++) {
      std::optional<T> value = getValue(array[i]);
      if (!value) {
        return std::nullopt;
      }

      result[i] = value ? *value : defaultValue;
    }

    return result;
  }

  // template <typename MatType>
  // static std::optional<MatType>
  // getMatNFromJsonValue(const CesiumUtility::JsonValue& value) {
  //  if (!jsonValue.isArray()) {
  //    return std::nullopt;
  //  }

  //  const CesiumUtility::JsonValue::Array& array = jsonValue.getArray();
  //  if (array.size() != N) {
  //    return std::nullopt;
  //  }

  //  glm::vec<N, T, Q> result;
  //  for (glm::length_t i = 0; i < N; i++) {
  //    std::optional<T> value = getValue(array[i]);
  //    if (!value) {
  //      return std::nullopt;
  //    }

  //    result[i] = value;
  //  }

  //  return result;
  //}

  template <typename ArrayType>
  static std::optional<PropertyArrayView<ArrayType>>
  getArrayValue(const CesiumUtility::JsonValue::Array& /*arrayValue*/) {
    return std::nullopt;
    // std::vector<ArrayType> values(arrayValue.size());
    // for (size_t i = 0; i < arrayValue.size(); i++) {
    //  const CesiumUtility::JsonValue& value = arrayValue[i];
    //  std::optional<ArrayType> arrayElement = getValue<ArrayType>(value);
    //  values[i] = arrayElement;
    //}

    // return PropertyArrayView<ArrayType>(values);
  }

  template <typename ArrayType>
  static PropertyArrayView<ArrayType> getArrayValueOrDefault(
      const CesiumUtility::JsonValue::Array& arrayValue,
      const ArrayType& defaultElementValue) {

    std::vector<ArrayType> values(arrayValue.size());
    for (size_t i = 0; i < arrayValue.size(); i++) {
      const CesiumUtility::JsonValue& value = arrayValue[i];
      std::optional<ArrayType> arrayElement = getValue<ArrayType>(value);
      values[i] = arrayElement;
    }

    return PropertyArrayView<ArrayType>(values);
  }
};

template <>
class PropertyView<PropertyArrayView<bool>>
    : IPropertyView<PropertyArrayView<bool>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
        _count(0),
        _required(false),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _count(0),
        _required(classProperty.required),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::BOOLEAN) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    _count = classProperty.count ? *classProperty.count : 0;

    if (!_required) {
      _defaultValue =
          classProperty.defaultProperty
              ? getBooleanArrayValues(*classProperty.defaultProperty)
              : std::nullopt;
    }
  }

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

  ~PropertyView() {}

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<bool>>
  offset() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<bool>>
  scale() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<bool>> max() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<bool>> min() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<bool>>
  noData() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<bool>>
  defaultValue() const noexcept override {
    return _defaultValue;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;
  int64_t _count;
  bool _required;
  std::optional<PropertyArrayView<bool>> _defaultValue;

  static std::optional<PropertyArrayView<bool>>
  getBooleanArrayValues(const CesiumUtility::JsonValue& value) {
    if (!value.isArray()) {
      return std::nullopt;
    }

    const CesiumUtility::JsonValue::Array& array = value.getArray();
    std::vector<bool> values(array.size());
    for (size_t i = 0; i < values.size(); i++) {
      if (!array[i].isBool()) {
        return std::nullopt;
      }

      values[i] = array[i].getBool();
    }

    return values;
  }
};

template <>
class PropertyView<PropertyArrayView<std::string_view>>
    : IPropertyView<PropertyArrayView<std::string_view>> {
public:
  /**
   * @brief Constructs an empty property instance.
   */
  PropertyView()
      : _propertyViewStatus(PropertyViewStatus::ErrorNonexistentProperty),
        _count(0),
        _required(false),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {}

  /**
   * @brief Constructs a property instance from a class definition only.
   */
  PropertyView(const ClassProperty& classProperty)
      : _propertyViewStatus(PropertyViewStatus::Valid),
        _count(0),
        _required(classProperty.required),
        _noData(std::nullopt),
        _defaultValue(std::nullopt) {
    if (classProperty.type != ClassProperty::Type::STRING) {
      _propertyViewStatus = PropertyViewStatus::ErrorTypeMismatch;
      return;
    }

    if (!classProperty.array) {
      _propertyViewStatus = PropertyViewStatus::ErrorArrayTypeMismatch;
      return;
    }

    _count = classProperty.count ? *classProperty.count : 0;

    // if (!_required) {
    //  _noData = classProperty.noData ?
    //  getStringArrayData(*classProperty.noData)
    //                                 : std::nullopt;
    //  _defaultValue = classProperty.defaultProperty
    //                      ? getStringArrayData(*classProperty.defaultProperty)
    //                      : std::nullopt;
    //}
  }

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

  ~PropertyView() {}

  /**
   * @copydoc IPropertyView::arrayCount
   */
  virtual int64_t arrayCount() const noexcept override { return _count; }

  /**
   * @copydoc IPropertyView::normalized
   */
  virtual bool normalized() const noexcept override { return false; }

  /**
   * @copydoc IPropertyView::offset
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  offset() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::scale
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  scale() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::max
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  max() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::min
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  min() const noexcept override {
    return std::nullopt;
  }

  /**
   * @copydoc IPropertyView::required
   */
  virtual bool required() const noexcept override { return _required; }

  /**
   * @copydoc IPropertyView::noData
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  noData() const noexcept override {
    return _noData;
  }

  /**
   * @copydoc IPropertyView::defaultValue
   */
  virtual std::optional<PropertyArrayView<std::string_view>>
  defaultValue() const noexcept override {
    return _defaultValue;
  }

protected:
  /**
   * @copydoc IPropertyView::propertyViewStatus
   */
  virtual PropertyViewStatus propertyViewStatus() const noexcept override {
    return _propertyViewStatus;
  }

private:
  PropertyViewStatus _propertyViewStatus;
  int64_t _count;
  bool _required;
  std::optional<PropertyArrayView<std::string_view>> _noData;
  std::optional<PropertyArrayView<std::string_view>> _defaultValue;

  static std::optional<PropertyArrayView<std::string_view>>
  getStringArrayData(const CesiumUtility::JsonValue& value) {
    if (!value.isArray()) {
      return std::nullopt;
    }

    std::vector<std::string> strings;
  }
};

} // namespace CesiumGltf
