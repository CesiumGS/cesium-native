#pragma once

#include "TilesetLoaderTestUtility.h"

#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <memory>
#include <string>

namespace Cesium3DTilesSelection {

TilesetContentLoaderResult<TilesetJsonLoader>
createTilesetJsonLoader(const std::filesystem::path& tilesetPath) {
  std::string tilesetPathStr = tilesetPath.string();
  auto pAccessor = std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
      std::map<
          std::string,
          std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>());
  auto externals = createMockJsonTilesetExternals(tilesetPathStr, pAccessor);
  auto loaderResultFuture =
      TilesetJsonLoader::createLoader(externals, tilesetPathStr, {});
  externals.asyncSystem.dispatchMainThreadTasks();

  return loaderResultFuture.wait();
}

TilesetContentLoaderResult<TilesetGltfLoader>
createTilesetGltfLoader(const std::filesystem::path& tilesetPath) {
  std::string tilesetPathStr = tilesetPath.string();
  auto pAccessor = std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
      std::map<
          std::string,
          std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>());
  auto externals = createMockJsonTilesetExternals(tilesetPathStr, pAccessor);
  auto loaderResultFuture =
      TilesetGltfLoader::createLoader(externals, tilesetPathStr, {});
  externals.asyncSystem.dispatchMainThreadTasks();

  return loaderResultFuture.wait();
}

TilesetExternals createMockJsonTilesetExternals(
    const std::string& tilesetPath,
    std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor>& pAssetAccessor) {
  auto tilesetContent = readFile(tilesetPath);
  auto pMockCompletedResponse =
      std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
          static_cast<uint16_t>(200),
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          std::move(tilesetContent));

  auto pMockCompletedRequest =
      std::make_shared<CesiumNativeTests::SimpleAssetRequest>(
          "GET",
          "tileset.json",
          CesiumAsync::HttpHeaders{},
          std::move(pMockCompletedResponse));

  pAssetAccessor->mockCompletedRequests.insert(
      {tilesetPath, std::move(pMockCompletedRequest)});

  auto pMockPrepareRendererResource =
      std::make_shared<SimplePrepareRendererResource>();

  auto pMockCreditSystem = std::make_shared<CesiumUtility::CreditSystem>();

  CesiumAsync::AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  return TilesetExternals{
      std::move(pAssetAccessor),
      std::move(pMockPrepareRendererResource),
      std::move(asyncSystem),
      std::move(pMockCreditSystem)};
}

TileLoadResult mockLoadTileContent(
    const std::filesystem::path& tilePath,
    TilesetContentLoader& loader,
    Tile& tile) {
  std::unique_ptr<CesiumNativeTests::SimpleAssetResponse>
      pMockCompletedResponse;
  if (std::filesystem::exists(tilePath)) {
    pMockCompletedResponse =
        std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(tilePath));
  } else {
    pMockCompletedResponse =
        std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
            static_cast<uint16_t>(404),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            std::vector<std::byte>{});
  }
  auto pMockCompletedRequest =
      std::make_shared<CesiumNativeTests::SimpleAssetRequest>(
          "GET",
          tilePath.filename().string(),
          CesiumAsync::HttpHeaders{},
          std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert(
      {tilePath.filename().string(), std::move(pMockCompletedRequest)});

  std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
          std::move(mockCompletedRequests));

  CesiumAsync::AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  TileLoadInput loadInput{
      tile,
      {},
      asyncSystem,
      pMockAssetAccessor,
      spdlog::default_logger(),
      {}};

  auto tileLoadResultFuture = loader.loadTileContent(loadInput);

  asyncSystem.dispatchMainThreadTasks();

  return tileLoadResultFuture.wait();
}

} // namespace Cesium3DTilesSelection
