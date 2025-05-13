#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>

#include <span>

namespace Cesium3DTilesSelection {

class Tile;

class CESIUM3DTILESSELECTION_API ITileSelectionEventReceiver {
public:
  /**
   * @brief A tile was previously visible, but now it is culled.
   *
   * @param tile The tile that became visible.
   * @param previousState The previous selection state of this tile.
   * @param currentState the current selection state of this tile.
   */
  virtual void tileVisible(
      const Tile& tile,
      const TileSelectionState& previousState,
      const TileSelectionState& currentState) = 0;

  /**
   * @brief A tile was previously culled, but now it is visible. It may be
   * refined or rendered.
   *
   * @param tile The tile that became culled.
   * @param previousState The previous selection state of this tile.
   * @param currentState the current selection state of this tile.
   */
  virtual void tileCulled(
      const Tile& tile,
      const TileSelectionState& previousState,
      const TileSelectionState& currentState) = 0;

  /**
   * @brief A tile was previously rendered, but now it has been refined. With
   * replacement refinement, this means that the `newRenderedTiles` are now
   * rendered instead of tile. With additive refinement, this means that the
   * `newRenderedTiles` are now rendered in addition to this tile.
   *
   * @param tile The tile that was refined.
   * @param previousState The previous selection state of this tile.
   * @param currentState the current selection state of this tile.
   * @param newRenderedTiles The new tiles that are rendered instead of or in
   * addition to this tile as a result of the refinement. This collection may be
   * empty if all children were culled or have no content.
   */
  virtual void tileRefined(
      const Tile& tile,
      const TileSelectionState& previousState,
      const TileSelectionState& currentState,
      const std::span<Tile*>& newRenderedTiles) = 0;

  /**
   * @brief A tile was previously rendered, but now its parent or other ancestor
   * is rendered instead.
   *
   * @param tile The tile that was coarsened.
   * @param previousState The previous selection state of this tile.
   * @param currentState the current selection state of this tile.
   * @param newRenderedTile The new ancestor tile that is rendered instead of
   * this tile.
   */
  virtual void tileCoarsened(
      const Tile& tile,
      const TileSelectionState& previousState,
      const TileSelectionState& currentState,
      const Tile& newRenderedTile) = 0;
};

} // namespace Cesium3DTilesSelection
