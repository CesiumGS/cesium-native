#pragma once

#include "Library.h"
#include "Tile.h"
#include "TileContentLoadResult.h"
#include "TileContentLoader.h"
#include "TileRefine.h"

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {
//
//class Tileset;
//
///**
// * @brief Creates a {@link TileContentLoadResult} from 3D Tiles external
// * `tileset.json` data.
// */
//class CESIUM3DTILESSELECTION_API ExternalTilesetContent final
//    : public TileContentLoader {
//public:
//  /**
//   * @copydoc TileContentLoader::load
//   *
//   * The result will only contain the `childTiles` and the `pNewTileContext`.
//   * Other fields will be empty or have default values.
//   */
//  CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
//  load(const TileContentLoadInput& input) override;
//
//private:
//  /**
//   * @brief Create the {@link TileContentLoadResult} from the given input data.
//   *
//   * @param pLogger The logger that receives details of loading errors and
//   * warnings.
//   * @param tileRefine The {@link TileRefine}
//   * @param url The source URL
//   * @param data The raw input data
//   * @return The {@link TileContentLoadResult}
//   */
//  static std::unique_ptr<TileContentLoadResult> load(
//      const std::shared_ptr<spdlog::logger>& pLogger,
//      const glm::dmat4& tileTransform,
//      TileRefine tileRefine,
//      const std::string& url,
//      const gsl::span<const std::byte>& data);
//};

} // namespace Cesium3DTilesSelection
