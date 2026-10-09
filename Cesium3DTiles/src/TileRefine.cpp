#include <Cesium3DTiles/TileRefine.h>
#include <CesiumTilesetJson/Tile.h>

#include <algorithm>

using namespace CesiumTilesetJson;

namespace Cesium3DTiles {

CesiumUtility::Result<TileRefine> parseTileRefine(const std::string& input) {
  if (input == Tile::Refine::ADD) {
    return CesiumUtility::Result<TileRefine>(TileRefine::Add);
  }
  if (input == Tile::Refine::REPLACE) {
    return CesiumUtility::Result<TileRefine>(TileRefine::Replace);
  }

  CesiumUtility::ErrorList errors;

  // Fallback for strings with improper capitalization.
  std::string inputUpper = input;
  std::transform(
      inputUpper.begin(),
      inputUpper.end(),
      inputUpper.begin(),
      [](unsigned char c) -> unsigned char {
        return static_cast<unsigned char>(std::toupper(c));
      });

  if (inputUpper == Tile::Refine::REPLACE || inputUpper == Tile::Refine::ADD) {
    std::string warning = "Tile refine value '" + input +
                          "' should be uppercase: '" + inputUpper + "'";
    errors.emplaceWarning(std::move(warning));

    TileRefine refine = (inputUpper == Tile::Refine::REPLACE)
                            ? TileRefine::Replace
                            : TileRefine::Add;
    return CesiumUtility::Result<TileRefine>(refine, std::move(errors));
  }

  std::string error = "Tile contained an unknown refine value: " + input;
  errors.emplaceError(std::move(error));
  return CesiumUtility::Result<TileRefine>(std::move(errors));
}

} // namespace Cesium3DTiles
