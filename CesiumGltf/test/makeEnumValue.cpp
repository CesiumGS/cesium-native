#include "makeEnumValue.h"

namespace CesiumNativeTests {
CesiumGltf::EnumValue makeEnumValue(const std::string& name, int64_t value) {
  CesiumGltf::EnumValue enumValue;
  enumValue.name = name;
  enumValue.value = value;
  return enumValue;
}
} // namespace CesiumNativeTests