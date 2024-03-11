#include <CesiumUtility/Uri.h>

#include <catch2/catch.hpp>

using namespace CesiumUtility;

TEST_CASE("Uri::getPath") {
  CHECK(Uri::getPath("https://example.com/") == "/");
  CHECK(Uri::getPath("https://example.com/foo/bar") == "/foo/bar");
  CHECK(Uri::getPath("https://example.com/foo/bar/") == "/foo/bar/");
  CHECK(Uri::getPath("https://example.com/?some=parameter") == "/");
  CHECK(
      Uri::getPath("https://example.com/foo/bar?some=parameter") == "/foo/bar");
  CHECK(
      Uri::getPath("https://example.com/foo/bar/?some=parameter") ==
      "/foo/bar/");
  CHECK(Uri::getPath("https://example.com") == "");
  CHECK(Uri::getPath("https://example.com?some=parameter") == "");
  CHECK(Uri::getPath("not a valid uri") == "");
}

TEST_CASE("Uri::setPath") {
  CHECK(Uri::setPath("https://example.com", "") == "https://example.com");
  CHECK(
      Uri::setPath("https://example.com?some=parameter", "") ==
      "https://example.com?some=parameter");
  CHECK(Uri::setPath("https://example.com", "/") == "https://example.com/");
  CHECK(
      Uri::setPath("https://example.com?some=parameter", "/") ==
      "https://example.com/?some=parameter");
  CHECK(
      Uri::setPath("https://example.com/foo?some=parameter", "/bar") ==
      "https://example.com/bar?some=parameter");
  CHECK(
      Uri::setPath("https://example.com/foo", "/bar") ==
      "https://example.com/bar");
  CHECK(
      Uri::setPath("https://example.com/foo?some=parameter", "/bar") ==
      "https://example.com/bar?some=parameter");
  CHECK(
      Uri::setPath("https://example.com/foo/", "/bar") ==
      "https://example.com/bar");
  CHECK(
      Uri::setPath("https://example.com/foo/?some=parameter", "/bar") ==
      "https://example.com/bar?some=parameter");
  CHECK(
      Uri::setPath("https://example.com/foo", "/bar/") ==
      "https://example.com/bar/");
  CHECK(
      Uri::setPath("https://example.com/foo?some=parameter", "/bar/") ==
      "https://example.com/bar/?some=parameter");
  CHECK(
      Uri::setPath("https://example.com/foo/bar", "/foo/bar") ==
      "https://example.com/foo/bar");
  CHECK(
      Uri::setPath("https://example.com/foo/bar?some=parameter", "/foo/bar") ==
      "https://example.com/foo/bar?some=parameter");
}
