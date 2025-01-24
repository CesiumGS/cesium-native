#include <CesiumUtility/Uri.h>

#include <doctest/doctest.h>

#include <string>

using namespace CesiumUtility;

TEST_CASE("Uri::getQueryValue") {
  std::string url = "https://example.com/?name=John&age=25";
  CHECK(Uri::getQueryValue(url, "name") == "John");
  CHECK(Uri::getQueryValue(url, "age") == std::to_string(25));
  CHECK(Uri::getQueryValue(url, "gender") == "");
  CHECK(Uri::getQueryValue(url, "") == "");
  CHECK(Uri::getQueryValue("", "name") == "");
  CHECK(
      Uri::getQueryValue(
          "https://example.com/?name=John&name=Jane&age=25",
          "name") == "John");
  CHECK(
      Uri::getQueryValue(
          "https://example.com/?name=John%20Doe&age=25",
          "name") == "John Doe");
  CHECK(Uri::getQueryValue("//example.com?value=1", "value") == "1");
}
