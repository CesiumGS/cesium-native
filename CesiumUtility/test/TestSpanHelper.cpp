#include <CesiumUtility/SpanHelper.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

TEST_CASE("reintepretCastSpan") {
  std::vector<std::byte> data(7 * sizeof(float));
  float* value = reinterpret_cast<float*>(data.data());
  value[0] = 0.0f;
  value[1] = 2.5f;
  value[2] = 1.0f;
  value[3] = 3.4f;
  value[4] = 0.7f;
  value[5] = 1.0f;
  value[6] = 2.9f;

  std::span<std::byte> byteView(data.data(), data.size());
  std::span<float> floatView =
      CesiumUtility::reintepretCastSpan<float>(byteView);
  REQUIRE(floatView.size() == 7);
  REQUIRE(floatView[0] == 0.0f);
  REQUIRE(floatView[1] == 2.5f);
  REQUIRE(floatView[2] == 1.0f);
  REQUIRE(floatView[3] == 3.4f);
  REQUIRE(floatView[4] == 0.7f);
  REQUIRE(floatView[5] == 1.0f);
  REQUIRE(floatView[6] == 2.9f);

  std::vector<int32_t> intData{1, -1};
  std::span<int32_t> intSpan = intData;
  std::span<uint32_t> uintSpan =
      CesiumUtility::reintepretCastSpan<uint32_t>(intSpan);
  REQUIRE(uintSpan.size() == 2);
  REQUIRE(uintSpan[0] == 1);
  REQUIRE(uintSpan[1] == uint32_t(-1));
}
