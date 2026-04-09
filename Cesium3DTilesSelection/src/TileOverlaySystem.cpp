#include "TileOverlaySystem.h"

namespace Cesium3DTilesSelection {

TileOverlaySystem::TileOverlaySystem(
    const LoadedTileEnumerator& loadedTiles,
    const TilesetExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : _collection(loadedTiles, externals, ellipsoid), _upsampler() {}

} // namespace Cesium3DTilesSelection
