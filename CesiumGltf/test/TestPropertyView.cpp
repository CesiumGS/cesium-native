#include "CesiumGltf/TPropertyView.h"
#include "catch2/catch.hpp"
#include <vector>
#include <cstddef>
#include <gsl/span>

template<typename T> static void checkNumeric(const std::vector<T>& expected) {
  std::vector<std::byte> data;
  data.resize(expected.size() * sizeof(T));
  std::memcpy(data.data(), expected.data(), data.size());

  CesiumGltf::TPropertyView<T> property(
      gsl::span<const std::byte>(data.data(), data.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      CesiumGltf::PropertyType::None,
      0,
      expected.size());

  for (size_t i = 0; i < property.size(); ++i) {
    REQUIRE(property[i] == expected[i]);
  }
}

TEST_CASE("Check create numeric property view") {
  SECTION("Uint8") { 
      std::vector<uint8_t> data{12, 33, 56, 67};
      checkNumeric(data);
  }

  SECTION("Int32") { 
      std::vector<int32_t> data{111222, -11133, -56000, 670000};
      checkNumeric(data);
  }

  SECTION("Float") { 
      std::vector<float> data{12.3333f, -12.44555f, -5.6111f, 6.7421f};
      checkNumeric(data);
  }

  SECTION("Double") { 
      std::vector<double> data{12222.3302121, -12000.44555, -5000.6113111, 6.7421};
      checkNumeric(data);
  }
}