#include "EncodeBase64String.h"
#include <catch2/catch.hpp>
#include <iostream>
#include <string_view>
#include <vector>

[[nodiscard]] std::vector<std::byte>
stringToByteVector(std::string_view view) noexcept {
  std::vector<std::byte> result;
  for (const auto c : view) {
    if (c != '\0') {
      result.emplace_back(std::byte(c));
    }
  }

  return result;
}

TEST_CASE("encodeAsBase64String returns empty string on empty vector input") {
  const std::vector<std::byte> empty;
  const auto result = CesiumGltf::encodeAsBase64String(empty);
  REQUIRE(result.empty());
}

TEST_CASE("encodeAsBase64String encodes HelloWorld! to 'SGVsbG9Xb3JsZCE='") {
  const std::vector<std::byte> helloWorld = stringToByteVector("HelloWorld!");
  const auto result = CesiumGltf::encodeAsBase64String(helloWorld);
  REQUIRE(result == "SGVsbG9Xb3JsZCE=");
}

TEST_CASE("encodeAsBase64 encodes appropriate padding") {
  const std::vector<std::byte> oneZero{std::byte(0)};
  REQUIRE(CesiumGltf::encodeAsBase64String(oneZero) == "AA==");

  std::vector<std::byte> fourZeros{
      std::byte(0),
      std::byte(0),
      std::byte(0),
      std::byte(0)};
  REQUIRE(CesiumGltf::encodeAsBase64String(fourZeros) == "AAAAAA==");
}
