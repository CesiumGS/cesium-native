#include "MockAssetAccessor.h"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "MockTaskProcessor.h"

#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace CesiumAsync;

namespace {

std::vector<std::byte> asBytes(const std::vector<int>& ints) {
  std::vector<std::byte> bytes(ints.size());

  for (size_t i = 0; i < ints.size(); ++i) {
    CHECK(ints[i] >= 0);
    CHECK(ints[i] < 255);
    bytes[i] = std::byte(ints[i]);
  }

  return bytes;
}

} // namespace

TEST_CASE("GunzipAssetAccessor") {
  SUBCASE("passes through responses without gzip header") {
    auto pAccessor = std::make_shared<GunzipAssetAccessor>(
        std::make_shared<MockAssetAccessor>(std::make_shared<MockAssetRequest>(
            "GET",
            "https://example.com",
            HttpHeaders{std::make_pair("Foo", "Bar")},
            std::make_unique<MockAssetResponse>(
                static_cast<uint16_t>(200),
                "Application/Whatever",
                HttpHeaders{std::make_pair("Some-Header", "in the response")},
                asBytes(std::vector<int>{0x01, 0x02, 0x03})))));

    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();
    AsyncSystem asyncSystem(mockTaskProcessor);

    auto pCompletedRequest =
        pAccessor->get(asyncSystem, "https://example.com", {}).wait();
    CHECK(pCompletedRequest->url() == "https://example.com");
    CHECK(pCompletedRequest->method() == "GET");
    CHECK(
        pCompletedRequest->headers() ==
        HttpHeaders{std::make_pair("Foo", "Bar")});

    const IAssetResponse* pResponse = pCompletedRequest->response();
    REQUIRE(pResponse != nullptr);
    CHECK(pResponse->statusCode() == 200);
    CHECK(pResponse->contentType() == "Application/Whatever");
    CHECK(
        pResponse->headers() ==
        HttpHeaders{std::make_pair("Some-Header", "in the response")});
    CHECK(
        std::vector<std::byte>(
            pResponse->data().data(),
            pResponse->data().data() + pResponse->data().size()) ==
        asBytes(std::vector<int>{0x01, 0x02, 0x03}));
  }

  SUBCASE("gunzips a gzipped responses") {
    auto pAccessor = std::make_shared<GunzipAssetAccessor>(
        std::make_shared<MockAssetAccessor>(std::make_shared<MockAssetRequest>(
            "GET",
            "https://example.com",
            HttpHeaders{std::make_pair("Foo", "Bar")},
            std::make_unique<MockAssetResponse>(
                static_cast<uint16_t>(200),
                "Application/Whatever",
                HttpHeaders{std::make_pair("Some-Header", "in the response")},
                asBytes(std::vector<int>{
                    0x1F, 0x8B, 0x08, 0x08, 0x34, 0xEE, 0x77, 0x64, 0x00, 0x03,
                    0x6F, 0x6E, 0x65, 0x74, 0x77, 0x6F, 0x74, 0x68, 0x72, 0x65,
                    0x65, 0x2E, 0x64, 0x61, 0x74, 0x00, 0x63, 0x64, 0x62, 0x06,
                    0x00, 0x1D, 0x80, 0xBC, 0x55, 0x03, 0x00, 0x00, 0x00})))));

    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();
    AsyncSystem asyncSystem(mockTaskProcessor);

    auto pCompletedRequest =
        pAccessor->get(asyncSystem, "https://example.com", {}).wait();
    CHECK(pCompletedRequest->url() == "https://example.com");
    CHECK(pCompletedRequest->method() == "GET");
    CHECK(
        pCompletedRequest->headers() ==
        HttpHeaders{std::make_pair("Foo", "Bar")});

    const IAssetResponse* pResponse = pCompletedRequest->response();
    REQUIRE(pResponse != nullptr);
    CHECK(pResponse->statusCode() == 200);
    CHECK(pResponse->contentType() == "Application/Whatever");
    CHECK(
        pResponse->headers() ==
        HttpHeaders{std::make_pair("Some-Header", "in the response")});
    CHECK(
        std::vector<std::byte>(
            pResponse->data().data(),
            pResponse->data().data() + pResponse->data().size()) ==
        asBytes(std::vector<int>{0x01, 0x02, 0x03}));
  }

  SUBCASE("passes through a response that has a gzip header but can't be "
          "gunzipped") {
    auto pAccessor = std::make_shared<GunzipAssetAccessor>(
        std::make_shared<MockAssetAccessor>(std::make_shared<MockAssetRequest>(
            "GET",
            "https://example.com",
            HttpHeaders{std::make_pair("Foo", "Bar")},
            std::make_unique<MockAssetResponse>(
                static_cast<uint16_t>(200),
                "Application/Whatever",
                HttpHeaders{std::make_pair("Some-Header", "in the response")},
                asBytes(std::vector<int>{0x1F, 0x8B, 0x01, 0x02, 0x03})))));

    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();
    AsyncSystem asyncSystem(mockTaskProcessor);

    auto pCompletedRequest =
        pAccessor->get(asyncSystem, "https://example.com", {}).wait();
    CHECK(pCompletedRequest->url() == "https://example.com");
    CHECK(pCompletedRequest->method() == "GET");
    CHECK(
        pCompletedRequest->headers() ==
        HttpHeaders{std::make_pair("Foo", "Bar")});

    const IAssetResponse* pResponse = pCompletedRequest->response();
    REQUIRE(pResponse != nullptr);
    CHECK(pResponse->statusCode() == 200);
    CHECK(pResponse->contentType() == "Application/Whatever");
    CHECK(
        pResponse->headers() ==
        HttpHeaders{std::make_pair("Some-Header", "in the response")});
    CHECK(
        std::vector<std::byte>(
            pResponse->data().data(),
            pResponse->data().data() + pResponse->data().size()) ==
        asBytes(std::vector<int>{0x1F, 0x8B, 0x01, 0x02, 0x03}));
  }

  SUBCASE("works with request method") {
    auto pAccessor = std::make_shared<GunzipAssetAccessor>(
        std::make_shared<MockAssetAccessor>(std::make_shared<MockAssetRequest>(
            "GET",
            "https://example.com",
            HttpHeaders{std::make_pair("Foo", "Bar")},
            std::make_unique<MockAssetResponse>(
                static_cast<uint16_t>(200),
                "Application/Whatever",
                HttpHeaders{std::make_pair("Some-Header", "in the response")},
                asBytes(std::vector<int>{
                    0x1F, 0x8B, 0x08, 0x08, 0x34, 0xEE, 0x77, 0x64, 0x00, 0x03,
                    0x6F, 0x6E, 0x65, 0x74, 0x77, 0x6F, 0x74, 0x68, 0x72, 0x65,
                    0x65, 0x2E, 0x64, 0x61, 0x74, 0x00, 0x63, 0x64, 0x62, 0x06,
                    0x00, 0x1D, 0x80, 0xBC, 0x55, 0x03, 0x00, 0x00, 0x00})))));

    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();
    AsyncSystem asyncSystem(mockTaskProcessor);

    auto pCompletedRequest = pAccessor
                                 ->request(
                                     asyncSystem,
                                     "GET",
                                     "https://example.com",
                                     {},
                                     std::vector<std::byte>())
                                 .wait();
    CHECK(pCompletedRequest->url() == "https://example.com");
    CHECK(pCompletedRequest->method() == "GET");
    CHECK(
        pCompletedRequest->headers() ==
        HttpHeaders{std::make_pair("Foo", "Bar")});

    const IAssetResponse* pResponse = pCompletedRequest->response();
    REQUIRE(pResponse != nullptr);
    CHECK(pResponse->statusCode() == 200);
    CHECK(pResponse->contentType() == "Application/Whatever");
    CHECK(
        pResponse->headers() ==
        HttpHeaders{std::make_pair("Some-Header", "in the response")});
    CHECK(
        std::vector<std::byte>(
            pResponse->data().data(),
            pResponse->data().data() + pResponse->data().size()) ==
        asBytes(std::vector<int>{0x01, 0x02, 0x03}));
  }
}
