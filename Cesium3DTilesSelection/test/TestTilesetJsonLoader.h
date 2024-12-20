#pragma once

#include "Cesium3DTilesSelection/Tile.h"
#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

namespace Cesium3DTilesSelection {

Cesium3DTilesSelection::TilesetExternals createMockJsonTilesetExternals(
    const std::string& tilesetPath,
    std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor>& pAssetAccessor);

TilesetContentLoaderResult<TilesetJsonLoader>
createTilesetJsonLoader(const std::filesystem::path& tilesetPath);

} // namespace Cesium3DTilesSelection
