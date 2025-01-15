#include <CesiumUtility/Uri.h>

#include <catch2/catch_test_macros.hpp>

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

  SECTION("handles unicode characters") {
    CHECK(Uri::getPath("http://example.com/🐶.bin") == "/🐶.bin");
    CHECK(Uri::getPath("http://example.com/示例测试用例") == "/示例测试用例");
    CHECK(Uri::getPath("http://example.com/Ῥόδος") == "/Ῥόδος");
    CHECK(Uri::getPath("http://example.com/🙍‍♂️🚪🤚/🪝🚗🚪/❓📞") == "/🙍‍♂️🚪🤚/🪝🚗🚪/❓📞");
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

  SECTION("handles unicode characters") {
    CHECK(Uri::setPath("http://example.com/foo/", "/🐶.bin") == "http://example.com/🐶.bin");
    CHECK(Uri::setPath("http://example.com/bar/", "/示例测试用例") == "http://example.com/示例测试用例");
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
      CesiumUtility::Uri::resolve(
          "//www.example.com",
          "/page/test",
          false,
          false) == "http://www.example.com/page/test");
  CHECK(
    CesiumUtility::Uri::resolve("https://www.example.com/", "/Ῥόδος") == "https://www.example.com/Ῥόδος"
  );
}

TEST_CASE("Uri::escape") {
  CHECK(Uri::escape("foo") == "foo");
  CHECK(Uri::escape("foo/bar") == "foo%2Fbar");
  CHECK(Uri::escape("🤞") == "%F0%9F%A4%9E");
}

TEST_CASE("Uri::unescape") {
  CHECK(Uri::unescape("foo") == "foo");
  CHECK(Uri::unescape("foo%2Fbar") == "foo/bar");
  CHECK(Uri::unescape("%F0%9F%A4%9E") == "🤞");
}

TEST_CASE("Uri::unixPathToUriPath") {
  CHECK(Uri::unixPathToUriPath("/wat") == "/wat");
  CHECK(Uri::unixPathToUriPath("wat") == "wat");
  CHECK(Uri::unixPathToUriPath("wat/the") == "wat/the");
  CHECK(Uri::unixPathToUriPath("/foo/bar") == "/foo/bar");
  CHECK(Uri::unixPathToUriPath("/some:file") == "/some%3Afile");
  CHECK(Uri::unixPathToUriPath("/🤞/😱/") == "/%F0%9F%A4%9E/%F0%9F%98%B1/");
}

TEST_CASE("Uri::windowsPathToUriPath") {
  CHECK(Uri::windowsPathToUriPath("c:\\wat") == "/c:/wat");
  CHECK(Uri::windowsPathToUriPath("c:/wat") == "/c:/wat");
  CHECK(Uri::windowsPathToUriPath("wat") == "wat");
  CHECK(Uri::windowsPathToUriPath("/foo/bar") == "/foo/bar");
  CHECK(Uri::windowsPathToUriPath("d:\\foo/bar\\") == "/d:/foo/bar/");
  CHECK(Uri::windowsPathToUriPath("e:\\some:file") == "/e:/some%3Afile");
  CHECK(
      Uri::windowsPathToUriPath("c:/🤞/😱/") ==
      "/c:/%F0%9F%A4%9E/%F0%9F%98%B1/");
  CHECK(
      Uri::windowsPathToUriPath("notadriveletter:\\file") ==
      "notadriveletter%3A/file");
  CHECK(
      Uri::windowsPathToUriPath("\\notadriveletter:\\file") ==
      "/notadriveletter%3A/file");
}

TEST_CASE("Uri::uriPathToUnixPath") {
  CHECK(Uri::uriPathToUnixPath("/wat") == "/wat");
  CHECK(Uri::uriPathToUnixPath("wat") == "wat");
  CHECK(Uri::uriPathToUnixPath("wat/the") == "wat/the");
  CHECK(Uri::uriPathToUnixPath("/foo/bar") == "/foo/bar");
  CHECK(Uri::uriPathToUnixPath("/some%3Afile") == "/some:file");
  CHECK(Uri::uriPathToUnixPath("/%F0%9F%A4%9E/%F0%9F%98%B1/") == "/🤞/😱/");
}

TEST_CASE("Uri::uriPathToWindowsPath") {
  CHECK(Uri::uriPathToWindowsPath("/c:/wat") == "c:\\wat");
  CHECK(Uri::uriPathToWindowsPath("wat") == "wat");
  CHECK(Uri::uriPathToWindowsPath("/foo/bar") == "\\foo\\bar");
  CHECK(Uri::uriPathToWindowsPath("/d:/foo/bar/") == "d:\\foo\\bar\\");
  CHECK(Uri::uriPathToWindowsPath("/e:/some%3Afile") == "e:\\some:file");
  CHECK(
      Uri::uriPathToWindowsPath("/c:/%F0%9F%A4%9E/%F0%9F%98%B1/") ==
      "c:\\🤞\\😱\\");
  CHECK(
      Uri::uriPathToWindowsPath("/notadriveletter:/file") ==
      "\\notadriveletter:\\file");
}
