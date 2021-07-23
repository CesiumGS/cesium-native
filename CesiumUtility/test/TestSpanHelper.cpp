#include "CesiumUtility/SpanHelper.h"
#include "catch2/catch.hpp"
#include <vector>

TEST_CASE("Test ReintepretCastSpan") {
  std::vector<std::byte> data(7 * sizeof(float));
  float* value = reinterpret_cast<float*>(data.data());
  value[0] = 0.0f;
  value[1] = 2.5f;
  value[2] = 1.0f;
  value[3] = 3.4f;
  value[4] = 0.7f;
  value[5] = 1.0f;
  value[6] = 2.9f;

  gsl::span<std::byte> byteView(data.data(), data.size());
  gsl::span<float> floatView =
      CesiumUtility::ReintepretCastSpan<float>(byteView);
  REQUIRE(floatView.size() == 7);
  REQUIRE(floatView[0] == 0.0f);
  REQUIRE(floatView[1] == 2.5f);
  REQUIRE(floatView[2] == 1.0f);
  REQUIRE(floatView[3] == 3.4f);
  REQUIRE(floatView[4] == 0.7f);
  REQUIRE(floatView[5] == 1.0f);
  REQUIRE(floatView[6] == 2.9f);
}
