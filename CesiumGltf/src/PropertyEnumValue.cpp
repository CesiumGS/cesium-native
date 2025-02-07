#include <CesiumGltf/Enum.h>
#include <CesiumGltf/EnumValue.h>
#include <CesiumGltf/PropertyEnumValue.h>

#include <algorithm>
#include <string_view>

std::optional<std::string_view>
CesiumGltf::PropertyEnumValue::name(const CesiumGltf::Enum& parentEnum) const {
  const auto found = std::find_if(
      parentEnum.values.begin(),
      parentEnum.values.end(),
      [value = this->_value](const CesiumGltf::EnumValue& enumValue) {
        return enumValue.value == value;
      });

  if (found == parentEnum.values.end()) {
    return std::nullopt;
  }

  return found->name;
}

bool CesiumGltf::operator==(
    const PropertyEnumValue lhs,
    const PropertyEnumValue rhs) {
  return lhs.value() == rhs.value();
}