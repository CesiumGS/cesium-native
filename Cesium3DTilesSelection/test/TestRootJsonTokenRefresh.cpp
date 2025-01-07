#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Uri.h>

#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

const std::string BasePrefix = "https://example.com/tileset/";
const std::string RootTilesetUrl = BasePrefix + "root.json";
const std::string ChildTilesetUrl = BasePrefix + "child.json";
const std::string ContentUrl = BasePrefix + "box.gltf";

std::vector<std::byte>
getJsonWithUrl(std::filesystem::path path, std::string url) {
  std::vector<std::byte> originalData = readFile(path);
  std::string fileStr = std::string(
      reinterpret_cast<char*>(originalData.data()),
      originalData.size());
  size_t placeholderPos = fileStr.find("{url}");
  if (placeholderPos == std::string::npos) {
    FAIL("Can't find placeholder {url} to replace");
  }

  fileStr.replace(placeholderPos, 5, url);
  std::vector<std::byte> output(
      reinterpret_cast<std::byte*>(fileStr.data()),
      reinterpret_cast<std::byte*>(fileStr.data() + fileStr.size()));
  return output;
}

class MockTokenAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
  MockTokenAssetAccessor() {}

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>&) override {
    if (url.starts_with(RootTilesetUrl)) {
      token++;
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(
              std::make_shared<SimpleAssetRequest>(
                  "GET",
                  RootTilesetUrl,
                  CesiumAsync::HttpHeaders{},
                  std::make_unique<SimpleAssetResponse>(
                      uint16_t(200),
                      "application/json",
                      CesiumAsync::HttpHeaders{},
                      getJsonWithUrl(
                          testDataPath / "RootTokenRefresh" / "root.json",
                          ChildTilesetUrl +
                              "?token=" + std::to_string(token))))));
    } else if (url.starts_with(ChildTilesetUrl)) {
      if (std::stoi(Uri::getQueryValue(url, "token")) < 2) {
        return asyncSystem.createResolvedFuture(
            std::shared_ptr<CesiumAsync::IAssetRequest>(
                std::make_shared<SimpleAssetRequest>(
                    "GET",
                    url,
                    HttpHeaders{},
                    std::make_unique<SimpleAssetResponse>(
                        uint16_t(400),
                        "doesn't matter",
                        HttpHeaders{},
                        std::vector<std::byte>()))));
      }

      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(
              std::make_shared<SimpleAssetRequest>(
                  "GET",
                  url,
                  CesiumAsync::HttpHeaders{},
                  std::make_unique<SimpleAssetResponse>(
                      uint16_t(200),
                      "application/json",
                      CesiumAsync::HttpHeaders{},
                      getJsonWithUrl(
                          testDataPath / "RootTokenRefresh" / "child.json",
                          ContentUrl)))));
    } else if (url.starts_with(ContentUrl)) {
      return asyncSystem.createResolvedFuture(
          std::shared_ptr<CesiumAsync::IAssetRequest>(
              std::make_shared<SimpleAssetRequest>(
                  "GET",
                  url,
                  CesiumAsync::HttpHeaders{},
                  std::make_unique<SimpleAssetResponse>(
                      uint16_t(200),
                      "model/gltf+json",
                      CesiumAsync::HttpHeaders{},
                      readFile(testDataPath / "gltf" / "box" / "Box.gltf")))));
    }

    FAIL("Cannot find request for url " << url);

    return asyncSystem.createResolvedFuture(
        std::shared_ptr<CesiumAsync::IAssetRequest>(nullptr));
  }

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& /* verb */,
      const std::string& url,
      const std::vector<THeader>& headers,
      const std::span<const std::byte>&) override {
    return this->get(asyncSystem, url, headers);
  }

  virtual void tick() noexcept override {}

private:
  int32_t token = 0;
};

} // namespace

TEST_CASE("Test Google-compatible token refresh") {
  auto externals = TilesetExternals{
      std::make_shared<MockTokenAssetAccessor>(),
      std::make_shared<SimplePrepareRendererResource>(),
      CesiumAsync::AsyncSystem(
          std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()),
      std::make_shared<CesiumUtility::CreditSystem>()};

  auto frustum = ViewState::create(
      glm::dvec3{-2693858, -4296814, 385502},
      glm::dvec3{0, 0, 1},
      glm::dvec3{0, 1, 0},
      glm::dvec2{1920, 1080},
      Math::OnePi,
      Math::OnePi,
      CesiumGeospatial::Ellipsoid::WGS84);
  auto frustums = std::vector<ViewState>{frustum};

  auto tileset = Tileset{externals, RootTilesetUrl};
  auto viewUpdateResult = tileset.updateView(frustums);
  viewUpdateResult = tileset.updateView(frustums);
}
