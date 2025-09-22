#include <CesiumUtility/TransformIterator.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

TEST_CASE("TransformIterator") {
  std::vector<int32_t> data = {1, 2, 3, 4, 5};
  auto transformFunction = [](int32_t x) { return double(x) - 0.5; };

  using IteratorType = CesiumUtility::TransformIterator<
      decltype(transformFunction),
      std::vector<int32_t>::iterator>;

  IteratorType it(transformFunction, data.begin());
  IteratorType end(transformFunction, data.end());

  std::vector<double> result;
  for (; it != end; ++it) {
    result.emplace_back(*it);
  }

  REQUIRE(result.size() == data.size());
  REQUIRE(result[0] == 0.5);
  REQUIRE(result[1] == 1.5);
  REQUIRE(result[2] == 2.5);
  REQUIRE(result[3] == 3.5);
  REQUIRE(result[4] == 4.5);
}
