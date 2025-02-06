#include <CesiumUtility/Uri.h>

#include <doctest/doctest.h>

#include <map>
#include <string>

using namespace CesiumUtility;

TEST_CASE("Uri::getPath") {
  SUBCASE("returns path") {
    CHECK(Uri::getPath("https://example.com/") == "/");
    CHECK(Uri::getPath("https://example.com/foo/bar") == "/foo/bar");
    CHECK(Uri::getPath("https://example.com/foo/bar/") == "/foo/bar/");
  }

  SUBCASE("ignores path parameters") {
    CHECK(Uri::getPath("https://example.com/?some=parameter") == "/");
    CHECK(
        Uri::getPath("https://example.com/foo/bar?some=parameter") ==
        "/foo/bar");
    CHECK(
        Uri::getPath("https://example.com/foo/bar/?some=parameter") ==
        "/foo/bar/");
    CHECK(
        Uri::getPath("geopackage:/home/courtyard_imagery.gpkg") ==
        "/home/courtyard_imagery.gpkg");
  }

  SUBCASE("returns / path for nonexistent paths") {
    CHECK(Uri::getPath("https://example.com") == "/");
    CHECK(Uri::getPath("https://example.com?some=parameter") == "/");
  }

  SUBCASE("handles unicode characters") {
    CHECK(Uri::getPath("http://example.com/🐶.bin") == "/%F0%9F%90%B6.bin");
    CHECK(
        Uri::getPath("http://example.com/示例测试用例") ==
        "/%E7%A4%BA%E4%BE%8B%E6%B5%8B%E8%AF%95%E7%94%A8%E4%BE%8B");
    CHECK(
        Uri::getPath("http://example.com/Ῥόδος") ==
        "/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82");
    CHECK(
        Uri::getPath(
            "http://example.com/🙍‍♂️🚪🤚/🪝🚗🚪/❓📞") ==
        "/%F0%9F%99%8D%E2%80%8D%E2%99%82%EF%B8%8F%F0%9F%9A%AA%F0%9F%A4%9A/"
        "%F0%9F%AA%9D%F0%9F%9A%97%F0%9F%9A%AA/%E2%9D%93%F0%9F%93%9E");
  }
}

TEST_CASE("Uri::setPath") {
  SUBCASE("sets empty path") {
    CHECK(Uri::setPath("https://example.com/", "") == "https://example.com/");
  }

  SUBCASE("sets new path") {
    CHECK(Uri::setPath("https://example.com/", "/") == "https://example.com/");
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

  SUBCASE("preserves path parameters") {
    CHECK(
        Uri::setPath("https://example.com?some=parameter", "") ==
        "https://example.com/?some=parameter");
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

  SUBCASE("sets same path") {
    CHECK(
        Uri::setPath("https://example.com/foo/bar", "/foo/bar") ==
        "https://example.com/foo/bar");
    CHECK(
        Uri::setPath(
            "https://example.com/foo/bar?some=parameter",
            "/foo/bar") == "https://example.com/foo/bar?some=parameter");
  }

  SUBCASE("handles unicode characters") {
    CHECK(
        Uri::setPath("http://example.com/foo/", "/🐶.bin") ==
        "http://example.com/%F0%9F%90%B6.bin");
    CHECK(
        Uri::setPath("http://example.com/bar/", "/示例测试用例") ==
        "http://example.com/"
        "%E7%A4%BA%E4%BE%8B%E6%B5%8B%E8%AF%95%E7%94%A8%E4%BE%8B");
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
          true) == "https://www.example.com/page/test");
  CHECK(
      CesiumUtility::Uri::resolve("https://www.example.com/", "/Ῥόδος") ==
      "https://www.example.com/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82");
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
  CHECK(Uri::unixPathToUriPath("/some:file") == "/some:file");
  CHECK(Uri::unixPathToUriPath("/🤞/😱/") == "/%F0%9F%A4%9E/%F0%9F%98%B1/");
}

TEST_CASE("Uri::windowsPathToUriPath") {
  CHECK(Uri::windowsPathToUriPath("c:\\wat") == "/c:/wat");
  CHECK(Uri::windowsPathToUriPath("c:/wat") == "/c:/wat");
  CHECK(Uri::windowsPathToUriPath("wat") == "wat");
  CHECK(Uri::windowsPathToUriPath("/foo/bar") == "/foo/bar");
  CHECK(Uri::windowsPathToUriPath("d:\\foo/bar\\") == "/d:/foo/bar/");
  CHECK(Uri::windowsPathToUriPath("e:\\some:file") == "/e:/some:file");
  CHECK(
      Uri::windowsPathToUriPath("c:/🤞/😱/") ==
      "/c:/%F0%9F%A4%9E/%F0%9F%98%B1/");
  CHECK(
      Uri::windowsPathToUriPath("notadriveletter:\\file") ==
      "notadriveletter:/file");
  CHECK(
      Uri::windowsPathToUriPath("\\notadriveletter:\\file") ==
      "/notadriveletter:/file");
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

TEST_CASE("Uri::addQuery") {
  CHECK(
      Uri::addQuery("https://example.com/", "a", "1") ==
      "https://example.com/?a=1");
  CHECK(
      Uri::addQuery("https://example.com/?a=1", "b", "2") ==
      "https://example.com/?a=1&b=2");
  CHECK(
      Uri::addQuery("https://example.com/?a=1", "a", "2") ==
      "https://example.com/?a=2");
  CHECK(
      Uri::addQuery("https://unparseable url", "a", "1") ==
      "https://unparseable url");
  CHECK(
      Uri::addQuery("https://example.com/", "a", "!@#$%^&()_+{}|") ==
      "https://example.com/?a=%21%40%23%24%25%5E%26%28%29_%2B%7B%7D%7C");
}

TEST_CASE("Uri::substituteTemplateParameters") {
  const std::map<std::string, std::string> params{
      {"a", "aValue"},
      {"b", "bValue"},
      {"c", "cValue"},
      {"s", "teststr"},
      {"one", "1"}};

  const auto substitutionCallback = [&params](const std::string& placeholder) {
    auto it = params.find(placeholder);
    return it == params.end() ? placeholder : it->second;
  };

  CHECK(
      Uri::substituteTemplateParameters(
          "https://example.com/{a}/{b}/{c}",
          substitutionCallback) == "https://example.com/aValue/bValue/cValue");
  CHECK(
      Uri::substituteTemplateParameters(
          "https://example.com/enco%24d%5Ee%2Fd{s}tr1n%25g",
          []([[maybe_unused]] const std::string& placeholder) {
            return "teststr";
          }) == "https://example.com/enco%24d%5Ee%2Fdteststrtr1n%25g");
  CHECK(
      Uri::substituteTemplateParameters(
          "https://example.com/{a",
          substitutionCallback) == "https://example.com/{a");
  CHECK(
      Uri::substituteTemplateParameters(
          "https://example.com/{}",
          substitutionCallback) == "https://example.com/");
  CHECK(
      Uri::substituteTemplateParameters(
          "https://example.com/a}",
          substitutionCallback) == "https://example.com/a}");
}

TEST_CASE("UriQuery") {
  SUBCASE("preserves placeholders") {
    Uri uri("https://example.com?query={whatever}&{this}={that}");
    UriQuery query(uri);

    CHECK(query.getValue("query") == "{whatever}");
    CHECK(query.getValue("{this}") == "{that}");

    query.setValue("query", "foo");
    query.setValue("{this}", "{another}");
    CHECK(query.getValue("query") == "foo");
    CHECK(query.getValue("{this}") == "{another}");

    CHECK(query.toQueryString() == "query=foo&%7Bthis%7D=%7Banother%7D");
  }
}
