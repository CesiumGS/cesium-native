#pragma once

#include <CesiumGltf/EnumValue.h>

#include <cstdint>
#include <string>

namespace CesiumNativeTests {
CesiumGltf::EnumValue makeEnumValue(const std::string& name, int64_t value);
}