#include <Cesium3DTilesSelection/ITwinRealityDataContentLoaderFactory.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>

#include <functional>
#include <memory>
#include <utility>

using namespace Cesium3DTilesSelection;

namespace {
struct ImmediateTaskProcessor final : CesiumAsync::ITaskProcessor {
  void startTask(std::function<void()> f) override { f(); }
};
} // namespace

TEST_CASE("ITwinRealityDataContentLoaderFactory") {
  CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<ImmediateTaskProcessor>());

  ITwinRealityDataContentLoaderFactory::TokenRefreshCallback refreshCb =
      [asyncSystem](const std::string& /*previousToken*/) mutable {
        CesiumUtility::Result<std::string> r(std::string("token"));
        return asyncSystem.createResolvedFuture(std::move(r));
      };

  SUBCASE("IsValid") {
    ITwinRealityDataContentLoaderFactory factory(
        "realityDataId",
        std::optional<std::string>("iTwinId"),
        "accessToken",
        std::move(refreshCb));
    CHECK(factory.isValid());
  }

  SUBCASE("IsValidWithAltURL") {
    ITwinRealityDataContentLoaderFactory factory(
        "realityDataId",
        std::optional<std::string>("iTwinId"),
        "accessToken",
        "baseURL",
        std::move(refreshCb));
    CHECK(factory.isValid());
  }

  SUBCASE("IsInvalidWithEmptyRealityDataId") {
    ITwinRealityDataContentLoaderFactory factory(
        "",
        std::optional<std::string>("iTwinId"),
        "accessToken",
        "baseURL",
        std::move(refreshCb));
    CHECK(!factory.isValid());
  }

  SUBCASE("IsInvalidWithEmptyAccessToken") {
    ITwinRealityDataContentLoaderFactory factory(
        "realityDataId",
        std::optional<std::string>("iTwinId"),
        "",
        "baseURL",
        std::move(refreshCb));
    CHECK(!factory.isValid());
  }

  SUBCASE("IsInvalidWithEmptyBaseURL") {
    ITwinRealityDataContentLoaderFactory factory(
        "realityDataId",
        std::optional<std::string>("iTwinId"),
        "accessToken",
        "",
        std::move(refreshCb));
    CHECK(!factory.isValid());
  }
}
