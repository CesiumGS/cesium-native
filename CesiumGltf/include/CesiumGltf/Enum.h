#pragma once

#include <CesiumGltf/EnumSpec.h>
#include <CesiumGltf/Library.h>

#include <algorithm>
#include <optional>
#include <string_view>

namespace CesiumGltf {
/** @copydoc EnumSpec */
struct CESIUMGLTF_API Enum final : public EnumSpec {
  Enum() = default;

  /**
   * @brief Obtains the name corresponding to the given enum integer value from
   * this enum definition.
   *
   * @param enumValue The enum value to resolve into a name.
   * @returns The name in the enum definition corresponding to this value, or an
   * empty string if the value cannot be found in the definition.
   */
  template <typename T>
  std::optional<std::string_view> getName(T enumValue) const {
    const auto found = std::find_if(
        this->values.begin(),
        this->values.end(),
        [value = static_cast<int64_t>(enumValue)](
            const CesiumGltf::EnumValue& e) { return e.value == value; });

    if (found == this->values.end()) {
      return std::nullopt;
    }

    return found->name;
  }
};
} // namespace CesiumGltf