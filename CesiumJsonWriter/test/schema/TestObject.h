#pragma once

#include <CesiumUtility/ExtensibleObject.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Test {

struct Property : public CesiumUtility::ExtensibleObject {
  std::optional<int64_t> intOptional;
  std::vector<double> floatArray;
};

struct TestExtension : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* TypeName = "TestExtension";
  static inline constexpr const char* ExtensionName = "TEST_EXTENSION";

  std::vector<Property> properties;
};

struct TestObject : public CesiumUtility::ExtensibleObject {
  static inline constexpr const char* TypeName = "TestObject";

  int64_t intProperty;
  std::string stringProperty;
  std::unordered_map<std::string, Property> additionalProperties;
};

} // namespace Test
