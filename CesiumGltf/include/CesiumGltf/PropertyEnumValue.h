#pragma once

#include <CesiumGltf/Enum.h>
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
class PropertyEnumValue {
public:
  /**
   * @brief Constructs an empty enum value.
   */
  PropertyEnumValue() : _value{} {}

  /**
   * @brief Constructs a \ref PropertyEnumValue from an integer value.
   *
   * @param value The integer value.
   */
  PropertyEnumValue(const int64_t value) noexcept : _value{value} {}

  /**
   * @brief Obtains the name of this enum value from the corresponding \ref
   * CesiumGltf::Enum.
   *
   * @param parentEnum The enum definition that this enum value corresponds to.
   * @returns The name in the enum definition corresponding to this value, or an
   * empty string if the value cannot be found in the definition.
   */
  std::string_view name(const CesiumGltf::Enum& parentEnum) const;

  /**
   * @brief Obtains the integer value from this \ref PropertyEnumValue.
   */
  int64_t value() const { return this->_value; }

private:
  int64_t _value;
};

/** @brief Compares a \ref PropertyEnumValue with a \ref
 * PropertyEnumValue. */
bool operator==(const PropertyEnumValue lhs, const PropertyEnumValue rhs);
} // namespace CesiumGltf