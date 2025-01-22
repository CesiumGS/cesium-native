#include "../src/parseLinkHeader.h"

#include <doctest/doctest.h>

#include <vector>

using namespace CesiumIonClient;

TEST_CASE("parseLinkHeader") {
  std::vector<Link> result =
      parseLinkHeader("<https://api.cesium.com/v2/"
                      "tokens?limit=100&page=3&sortBy=NAME&search=cesium%"
                      "20token>; rel=\"next\", "
                      "<https://api.cesium.com/v2/"
                      "tokens?limit=100&page=1&sortBy=NAME&search=cesium%"
                      "20token>; rel=\"prev\"");
  REQUIRE(result.size() == 2);
  CHECK(
      result[0].url ==
      "https://api.cesium.com/v2/"
      "tokens?limit=100&page=3&sortBy=NAME&search=cesium%20token");
  CHECK(result[0].rel == "next");
  CHECK(
      result[1].url ==
      "https://api.cesium.com/v2/"
      "tokens?limit=100&page=1&sortBy=NAME&search=cesium%20token");
  CHECK(result[1].rel == "prev");
}
