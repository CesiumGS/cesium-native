#include <CesiumUtility/Uri.h>

#include <catch2/catch.hpp>

using namespace CesiumUtility;

TEST_CASE("Uri::getPath") {
  SECTION("returns path") {
    CHECK(Uri::getPath("https://example.com/") == "/");
    CHECK(Uri::getPath("https://example.com/foo/bar") == "/foo/bar");
    CHECK(Uri::getPath("https://example.com/foo/bar/") == "/foo/bar/");
  }

  SECTION("ignores path parameters") {
    CHECK(Uri::getPath("https://example.com/?some=parameter") == "/");
    CHECK(
        Uri::getPath("https://example.com/foo/bar?some=parameter") ==
        "/foo/bar");
    CHECK(
        Uri::getPath("https://example.com/foo/bar/?some=parameter") ==
        "/foo/bar/");
  }

  SECTION("returns empty path for nonexistent paths") {
    CHECK(Uri::getPath("https://example.com") == "");
    CHECK(Uri::getPath("https://example.com?some=parameter") == "");
  }

  SECTION("returns empty path for invalid uri") {
    CHECK(Uri::getPath("not a valid uri") == "");
  }
}

TEST_CASE("Uri::setPath") {
  SECTION("sets empty path") {
    CHECK(Uri::setPath("https://example.com", "") == "https://example.com");
  }

  SECTION("sets new path") {
    CHECK(Uri::setPath("https://example.com", "/") == "https://example.com/");
    CHECK(
        Uri::setPath("https://example.com/foo", "/bar") ==
        "https://example.com/bar");
    CHECK(
        Uri::setPath("https://example.com/foo/", "/bar") ==
        "https://example.com/bar");
    CHECK(
        Uri::setPath("https://example.com/foo", "/bar/") ==
        "https://example.com/bar/");
  }

  SECTION("preserves path parameters") {
    CHECK(
        Uri::setPath("https://example.com?some=parameter", "") ==
        "https://example.com?some=parameter");
    CHECK(
        Uri::setPath("https://example.com?some=parameter", "/") ==
        "https://example.com/?some=parameter");
    CHECK(
        Uri::setPath("https://example.com/foo?some=parameter", "/bar") ==
        "https://example.com/bar?some=parameter");
    CHECK(
        Uri::setPath("https://example.com/foo/?some=parameter", "/bar") ==
        "https://example.com/bar?some=parameter");
    CHECK(
        Uri::setPath("https://example.com/foo?some=parameter", "/bar/") ==
        "https://example.com/bar/?some=parameter");
  }

  SECTION("sets same path") {
    CHECK(
        Uri::setPath("https://example.com/foo/bar", "/foo/bar") ==
        "https://example.com/foo/bar");
    CHECK(
        Uri::setPath(
            "https://example.com/foo/bar?some=parameter",
            "/foo/bar") == "https://example.com/foo/bar?some=parameter");
  }

  SECTION("returns empty path for invalid uri") {
    CHECK(Uri::setPath("not a valid uri", "/foo/") == "");
  }
}

TEST_CASE("Uri::resolve") {
  CHECK(
      CesiumUtility::Uri::resolve("https://www.example.com/", "/page/test") ==
      "https://www.example.com/page/test");
  CHECK(
      CesiumUtility::Uri::resolve("//www.example.com", "/page/test") ==
      "https://www.example.com/page/test");
  CHECK(
      CesiumUtility::Uri::resolve("://www.example.com", "/page/test") ==
      "https://www.example.com/page/test");
}
