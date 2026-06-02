#include "ITwinRealityDataContentLoader.h"

#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace Cesium3DTilesSelection;

namespace {

static const std::string testBaseUrl = "https://api.test.com/";
static const std::string testRealityDataId = "123";
static const std::string testToken = "TOKEN";
static const std::string testITwinId = "itwin-42";

static const std::string testMetadataJson =
    "{\"realityData\":{\"id\":\"" + testRealityDataId +
    "\",\"rootDocument\":\"tileset.json\",\"type\":\"Cesium3DTiles\"}}";

static const std::string testReadaccessJson =
    "{\"_links\":{\"containerUrl\":{\"href\":\"https://cdn.test.com/"
    "data\"}}}";

static const std::string testTilesetJson =
    "{\"asset\":{\"version\":\"1.0\"},\"geometricError\":0,"
    "\"root\":{\"boundingVolume\":{\"sphere\":[0,0,0,1]},"
    "\"geometricError\":0,\"refine\":\"ADD\","
    "\"content\":{\"uri\":\"tile.b3dm\"}}}";

struct ImmediateTaskProcessor final : CesiumAsync::ITaskProcessor {
  void startTask(std::function<void()> f) override { f(); }
};

class FakeResponse final : public CesiumAsync::IAssetResponse {
public:
  FakeResponse(uint16_t status, std::string body) : _status(status) {
    for (char c : body) {
      _bytes.push_back(static_cast<std::byte>(c));
    }
  }

  uint16_t statusCode() const override { return _status; }

  std::string contentType() const override { return "application/json"; }

  const CesiumAsync::HttpHeaders& headers() const override { return _headers; }

  std::span<const std::byte> data() const override {
    return std::span<const std::byte>(_bytes.data(), _bytes.size());
  }

private:
  uint16_t _status;
  CesiumAsync::HttpHeaders _headers;
  std::vector<std::byte> _bytes;
};

class FakeRequest final : public CesiumAsync::IAssetRequest {
public:
  FakeRequest(
      std::string method,
      std::string url,
      CesiumAsync::HttpHeaders headers,
      std::shared_ptr<CesiumAsync::IAssetResponse> response)
      : _method(std::move(method)),
        _url(std::move(url)),
        _headers(std::move(headers)),
        _response(std::move(response)) {}

  const std::string& method() const override { return _method; }

  const std::string& url() const override { return _url; }

  const CesiumAsync::HttpHeaders& headers() const override { return _headers; }

  const CesiumAsync::IAssetResponse* response() const override {
    return _response.get();
  }

private:
  std::string _method;
  std::string _url;
  CesiumAsync::HttpHeaders _headers;
  std::shared_ptr<CesiumAsync::IAssetResponse> _response;
};

class RecordingAssetAccessor final : public CesiumAsync::IAssetAccessor {
public:
  using THeader = CesiumAsync::IAssetAccessor::THeader;

  std::vector<std::string> urls;
  std::vector<std::vector<THeader>> headersHistory;

  std::string metadataJson;
  std::string readaccessJson;
  std::string tilesetJson;

  int callCount = 0;

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override {
    urls.push_back(url);
    headersHistory.push_back(headers);
    std::shared_ptr<CesiumAsync::IAssetResponse> response;
    if (callCount == 0) {
      response = std::make_shared<FakeResponse>(200, metadataJson);
    } else if (callCount == 1) {
      response = std::make_shared<FakeResponse>(200, readaccessJson);
    } else {
      response = std::make_shared<FakeResponse>(200, tilesetJson);
    }
    ++callCount;
    CesiumAsync::HttpHeaders httpHeaders;
    for (const auto& h : headers) {
      httpHeaders[h.first] = h.second;
    }
    std::shared_ptr<CesiumAsync::IAssetRequest> req =
        std::make_shared<FakeRequest>(
            "GET",
            url,
            std::move(httpHeaders),
            std::move(response));
    return asyncSystem.createResolvedFuture(std::move(req));
  }

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string&,
      const std::string&,
      const std::vector<THeader>& = {},
      const std::span<const std::byte>& = {}) override {
    return asyncSystem.createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>());
  }

  void tick() noexcept override {}

  void reset() {
    urls.clear();
    headersHistory.clear();
    callCount = 0;
  }
};

static bool hasHeader(
    const std::vector<RecordingAssetAccessor::THeader>& headers,
    const std::string& key,
    const std::string& value) {
  for (const auto& h : headers) {
    if (h.first == key && h.second == value) {
      return true;
    }
  }
  return false;
}

} // namespace

TEST_CASE("ITwinRealityDataContentLoader") {

  auto asyncSystem =
      CesiumAsync::AsyncSystem(std::make_shared<ImmediateTaskProcessor>());

  auto accessor = std::make_shared<RecordingAssetAccessor>();

  accessor->metadataJson = testMetadataJson;
  accessor->readaccessJson = testReadaccessJson;
  accessor->tilesetJson = testTilesetJson;

  TilesetExternals externals{
      accessor,
      nullptr,
      asyncSystem,
      nullptr,
      spdlog::default_logger(),
      nullptr,
      nullptr,
      nullptr};

  auto refreshCallback = [asyncSystem](const std::string&) mutable {
    return asyncSystem.createResolvedFuture(
        CesiumUtility::Result<std::string>("new-token"));
  };

  auto run = [&](const std::optional<std::string>& itwinId) {
    accessor->reset();
    return ITwinRealityDataContentLoader::createLoader(
               externals,
               testRealityDataId,
               itwinId,
               testToken,
               testBaseUrl,
               refreshCallback,
               CesiumGeospatial::Ellipsoid::WGS84)
        .wait();
  };

  SUBCASE("Metadata URL") {
    auto loader = run(testITwinId);

    REQUIRE(accessor->urls.size() >= 1);
    CHECK(
        accessor->urls[0] ==
        testBaseUrl + testRealityDataId + "?iTwinId=" + testITwinId);
  }

  SUBCASE("Metadata URL without iTwinId") {
    auto loader = run(std::nullopt);

    REQUIRE(accessor->urls.size() >= 1);
    CHECK(accessor->urls[0] == testBaseUrl + testRealityDataId);
  }

  SUBCASE("Readaccess URL") {
    auto loader = run(testITwinId);

    REQUIRE(accessor->urls.size() >= 2);
    CHECK(
        accessor->urls[1] ==
        testBaseUrl + testRealityDataId + "/readaccess?iTwinId=" + testITwinId);
  }

  SUBCASE("Headers") {
    auto loader = run(testITwinId);

    REQUIRE(!accessor->headersHistory.empty());

    const auto& headers = accessor->headersHistory[0];

    CHECK(hasHeader(headers, "Authorization", "Bearer " + testToken));
    CHECK(hasHeader(
        headers,
        "Accept",
        "application/vnd.bentley.itwin-platform.v1+json"));
  }
}
