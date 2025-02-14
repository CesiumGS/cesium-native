#pragma once

#include <cstdint>

namespace CesiumGltf {

/**
 * @brief An enum metadata value of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Contains the integer value of the enum as well as the name of the value.
 */
class PropertyEnumValue {
public:
  /**
   * @brief Constructs an empty enum value.
   */
  PropertyEnumValue() = default;

  /**
   * @brief Constructs a \ref PropertyEnumValue from an integer value.
   *
   * @param value The integer value.
   */
  PropertyEnumValue(const int64_t value) noexcept : _value{value} {}

  /**
   * @brief Obtains the integer value from this \ref PropertyEnumValue.
   */
  int64_t value() const { return this->_value; }

private:
  int64_t _value = -1;
};

/** @brief Compares a \ref PropertyEnumValue with a \ref
 * PropertyEnumValue. */
bool operator==(const PropertyEnumValue lhs, const PropertyEnumValue rhs);
} // namespace CesiumGltf