#include <Cesium3DTiles/TileRefine.h>
#include <CesiumGltf/ExtensionNode3dTilesTileset.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Result.h>

#include <algorithm>
#include <cctype>
#include <string>

using namespace CesiumGltf;

namespace Cesium3DTiles {

CesiumUtility::Result<TileRefine> getTileRefine(const std::string& input) {
  if (input == ExtensionNode3dTilesTileset::Refine::ADD) {
    return CesiumUtility::Result<TileRefine>(TileRefine::Add);
  }
  if (input == ExtensionNode3dTilesTileset::Refine::REPLACE) {
    return CesiumUtility::Result<TileRefine>(TileRefine::Replace);
  }

  // Fallback for strings with improper capitalization.
  std::string inputUpper = input;
  std::transform(
      inputUpper.begin(),
      inputUpper.end(),
      inputUpper.begin(),
      [](unsigned char c) -> unsigned char {
        return static_cast<unsigned char>(std::toupper(c));
      });

  if (inputUpper == ExtensionNode3dTilesTileset::Refine::REPLACE ||
      inputUpper == ExtensionNode3dTilesTileset::Refine::ADD) {
    TileRefine refine =
        (inputUpper == ExtensionNode3dTilesTileset::Refine::REPLACE)
            ? TileRefine::Replace
            : TileRefine::Add;
    return CesiumUtility::Result<TileRefine>(
        refine,
        CesiumUtility::ErrorList::warning(fmt::format(
            "Tile refine value '{}' should be uppercase: '{}'",
            input,
            inputUpper)));
  }

  return CesiumUtility::Result<TileRefine>(CesiumUtility::ErrorList::error(
      fmt::format("Tile contained an unknown refine value: {}", input)));
}

} // namespace Cesium3DTiles
