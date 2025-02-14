#pragma once

#include <CesiumGltf/EnumSpec.h>
#include <CesiumGltf/Library.h>
#include <CesiumGltf/PropertyEnumValue.h>

#include <optional>
#include <string_view>

namespace CesiumGltf {
/** @copydoc EnumSpec */
struct CESIUMGLTF_API Enum final : public EnumSpec {
  Enum() = default;

  /**
   * @brief Obtains the name of the given \ref PropertyEnumValue from this enum
   * definition.
   *
   * @param enumValue The \ref PropertyEnumValue to resolve into a name.
   * @returns The name in the enum definition corresponding to this value, or an
   * empty string if the value cannot be found in the definition.
   */
  std::optional<std::string_view>
  getName(const CesiumGltf::PropertyEnumValue& enumValue) const;
};
} // namespace CesiumGltf
