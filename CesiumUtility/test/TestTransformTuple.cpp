#include <CesiumUtility/transformTuple.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

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

  SUBCASE("Transforms a const tuple") {
    const std::tuple<std::string, std::vector<int>> tuple =
        std::make_tuple(std::string("test"), std::vector<int>{1, 2, 3});

    struct Transformer {
      size_t operator()(const std::string& str) const { return str.size(); }

      size_t operator()(const std::vector<int>& v) const { return v.size(); }
    };

    std::string firstResult;
    std::tuple<size_t, size_t> transformedTuple =
        transformTuple(tuple, Transformer());
    CHECK_EQ(std::get<0>(transformedTuple), 4);
    CHECK_EQ(std::get<1>(transformedTuple), 3);
  }

  SUBCASE("Transforms a moved tuple") {
    std::tuple<std::string, std::vector<int>> tuple =
        std::make_tuple(std::string("test"), std::vector<int>{1, 2, 3});

    struct Transformer {
      size_t operator()(std::string&& str) const { return str.size(); }

      size_t operator()(std::vector<int>&& v) const { return v.size(); }
    };

    std::string firstResult;
    std::tuple<size_t, size_t> transformedTuple =
        transformTuple(std::move(tuple), Transformer());
    CHECK_EQ(std::get<0>(transformedTuple), 4);
    CHECK_EQ(std::get<1>(transformedTuple), 3);
  }
}
