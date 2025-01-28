#pragma once

#include <CesiumGltf/EnumValue.h>
#include <CesiumUtility/SpanHelper.h>

#include <string_view>

namespace CesiumGltf {

/**
 * @brief An enum metadata value of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Contains the integer value of the enum as well as the name of the value.
 */
template <typename ElementType> class PropertyEnumValue {
public:
  /**
   * @brief Constructs an empty enum value.
   */
  PropertyEnumValue() : _value{}, _name{} {}

  /**
   * @brief Constructs a \ref PropertyEnumValue from a name and value.
   *
   * @param name The name of this enum value.
   * @param value The integer value.
   */
  PropertyEnumValue(const std::string& name, const ElementType& value) noexcept
      : _value{value}, _name{name} {}

  std::string_view name() const { return this->_name; }

  ElementType value() const { return this->_value; }

private:
  ElementType _value;
  std::string _name;
};

/** @brief Compares a \ref PropertyEnumValue with a \ref
 * PropertyEnumValue. */
template <typename T>
bool operator==(
    const PropertyEnumValue<T>& lhs,
    const PropertyEnumValue<T>& rhs) {
  return lhs.value() == rhs.value() && lhs.name() == rhs.name();
}
} // namespace CesiumGltf