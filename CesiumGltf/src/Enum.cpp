#include <CesiumGltf/Enum.h>

#include <algorithm>
#include <optional>
#include <string_view>

std::optional<std::string_view>
CesiumGltf::Enum::getName(const PropertyEnumValue& enumValue) const {
  const auto found = std::find_if(
      this->values.begin(),
      this->values.end(),
      [value = enumValue.value()](const CesiumGltf::EnumValue& enumValue) {
        return enumValue.value == value;
      });

  if (found == this->values.end()) {
    return std::nullopt;
  }

  return found->name;
}