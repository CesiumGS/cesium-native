#pragma once

#include "TilesetGltfLoader.h"
#include "TilesetJsonLoader.h"

#include <CesiumNativeTests/SimpleAssetAccessor.h>

#include <filesystem>
#include <memory>
#include <string>

namespace CesiumNativeTests {
class SimpleAssetAccessor;
} // namespace CesiumNativeTests

namespace Cesium3DTilesSelection {

struct TileLoadResult;
class Tile;
class TilesetContentLoader;

TilesetExternals createMockJsonTilesetExternals(
    const std::string& tilesetPath,
    std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor>& pAssetAccessor);

TilesetContentLoaderResult<TilesetJsonLoader>
createTilesetJsonLoader(const std::filesystem::path& tilesetPath);

TilesetContentLoaderResult<TilesetGltfLoader>
createTilesetGltfLoader(const std::filesystem::path& tilesetPath);

TileLoadResult mockLoadTileContent(
    const std::filesystem::path& tilePath,
    TilesetContentLoader& loader,
    Tile& tile);

} // namespace Cesium3DTilesSelection
