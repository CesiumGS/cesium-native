#include <CesiumUtility/transformTuple.h>

#include <doctest/doctest.h>

#include <string>

using namespace CesiumUtility;

TEST_CASE("transformTuple") {
  SUBCASE("Transforms each element of a tuple") {
    auto tuple = std::make_tuple(100, -42, int64_t(97));

    std::string firstResult;
    std::tuple<std::string, std::string, std::string> transformedTuple =
        transformTuple(tuple, [&firstResult](const auto& element) {
          std::string result = std::to_string(element);
          if (firstResult.empty()) {
            firstResult = result;
          }
          return result;
        });

    CHECK_EQ(std::get<0>(transformedTuple), "100");
    CHECK_EQ(std::get<1>(transformedTuple), "-42");
    CHECK_EQ(std::get<2>(transformedTuple), "97");
    CHECK_EQ(firstResult, std::get<0>(transformedTuple));
  }
}
