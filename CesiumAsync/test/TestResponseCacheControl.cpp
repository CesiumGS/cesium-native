#include "ResponseCacheControl.h"

#include <catch2/catch.hpp>

using namespace CesiumAsync;

TEST_CASE("Test parsing cache-control header") {
  SECTION("Header has no cache-control header") {
    HttpHeaders responseHeader{
        {"Response-Header-1", "Response-Value-1"},
        {"Response-Header-2", "Response-Value-2"},
    };
    std::optional<ResponseCacheControl> cacheControl =
        ResponseCacheControl::parseFromResponseHeaders(responseHeader);
    REQUIRE(cacheControl == std::nullopt);
  }

  SECTION("Header has cache-control header") {
    HttpHeaders responseHeader{
        {"Cache-Control",
         "Must-Revalidate, No-Cache, No-Store, No-Transform, Public, Private, "
         "Proxy-Revalidate, Max-Age = 1000,  S-Maxage = 10"},
    };

    std::optional<ResponseCacheControl> cacheControl =
        ResponseCacheControl::parseFromResponseHeaders(responseHeader);
    REQUIRE(cacheControl != std::nullopt);
    REQUIRE(cacheControl->mustRevalidate());
    REQUIRE(cacheControl->noCache());
    REQUIRE(cacheControl->noStore());
    REQUIRE(cacheControl->noTransform());
    REQUIRE(cacheControl->accessControlPublic());
    REQUIRE(cacheControl->accessControlPrivate());
    REQUIRE(cacheControl->proxyRevalidate());
    REQUIRE(cacheControl->maxAge() == 1000);
    REQUIRE(cacheControl->sharedMaxAge() == 10);
  }

  SECTION("Header has cache-control header with only some directive") {
    HttpHeaders responseHeader{
        {"Cache-Control",
         "Must-Revalidate, No-Cache, No-Store, Public, Private, Max-Age = "
         "1000,  S-Maxage = 10"},
    };

    std::optional<ResponseCacheControl> cacheControl =
        ResponseCacheControl::parseFromResponseHeaders(responseHeader);
    REQUIRE(cacheControl != std::nullopt);
    REQUIRE(cacheControl->mustRevalidate());
    REQUIRE(cacheControl->noCache());
    REQUIRE(cacheControl->noStore());
    REQUIRE(cacheControl->noTransform() == false);
    REQUIRE(cacheControl->accessControlPublic());
    REQUIRE(cacheControl->accessControlPrivate());
    REQUIRE(cacheControl->proxyRevalidate() == false);
    REQUIRE(cacheControl->maxAge() == 1000);
    REQUIRE(cacheControl->sharedMaxAge() == 10);
  }
}
