#pragma once

#include <Cesium3DTilesSelection/Library.h>

#include <cstdint>

namespace Cesium3DTilesSelection {

/**
 * @brief A description of the selection state of a {@link Tile} during the
 * {@link Tileset::updateViewGroup} process.
 *
 * Instances of this class are stored in a {@link TilesetViewGroup} for each
 * visited {@link Tile}, and are used to track the state of the tile during the
 * process of selecting tiles for rendering. The {@link Tileset} updates this
 * state while traversing the tile hierarchy, tracking whether a tile was
 * rendered, culled, or refined in the last frame.
 */
class TileSelectionState final {
public:
  /**
   * @brief Enumeration of possible results of a {@link TileSelectionState}
   */
  enum class CESIUM3DTILESSELECTION_API Result {
    /**
     * @brief There was no selection result.
     *
     * This may be the case when the tile wasn't visited last frame.
     */
    None = 0,

    /**
     * @brief This tile was deemed not visible and culled.
     */
    Culled = 1,

    /**
     * @brief The tile was selected for rendering.
     */
    Rendered = 2,

    /**
     * @brief This tile did not meet the required screen-space error and was
     * refined.
     */
    Refined = 3,

    /**
     * @brief This tile was rendered but then removed from the render list
     *
     * This tile was originally rendered, but it got kicked out of the render
     * list in favor of an ancestor because some tiles in its subtree were not
     * yet renderable.
     */
    RenderedAndKicked = 4,

    /**
     * @brief This tile was refined but then removed from the render list
     *
     * This tile was originally refined, but its rendered descendants got kicked
     * out of the render list in favor of an ancestor because some tiles in its
     * subtree were not yet renderable.
     */
    RefinedAndKicked = 5
  };

  /**
   * @brief Initializes a new instance with
   * {@link TileSelectionState::Result::None}
   */
  constexpr TileSelectionState() noexcept : _result(Result::None) {}

  /**
   * @brief Initializes a new instance with a given
   * {@link TileSelectionState::Result}.
   *
   * @param result The result of the selection.
   */
  constexpr TileSelectionState(Result result) noexcept : _result(result) {}

  /**
   * @brief Gets the result of selection.
   *
   * @return The {@link TileSelectionState::Result}
   */
  constexpr Result getResult() const noexcept { return this->_result; }

  /**
   * @brief Determines if this tile or its descendents were kicked from the
   * render list.
   *
   * In other words, if its last selection result was
   * {@link TileSelectionState::Result::RenderedAndKicked} or
   * {@link TileSelectionState::Result::RefinedAndKicked}.
   *
   * @return `true` if the tile was kicked, and `false` otherwise
   */
  constexpr bool wasKicked() const noexcept {
    const Result result = this->getResult();
    return result == Result::RenderedAndKicked ||
           result == Result::RefinedAndKicked;
  }

  /**
   * @brief Gets the original selection result prior to being kicked.
   *
   * If the tile wasn't kicked, the original value is returned.
   *
   * @return The {@link TileSelectionState::Result} prior to being kicked.
   */
  constexpr Result getOriginalResult() const noexcept {
    const Result result = this->getResult();

    switch (result) {
    case Result::RefinedAndKicked:
      return Result::Refined;
    case Result::RenderedAndKicked:
      return Result::Rendered;
    default:
      return result;
    }
  }

  /**
   * @brief Marks this tile as "kicked".
   */
  constexpr void kick() noexcept {
    switch (this->_result) {
    case Result::Rendered:
      this->_result = Result::RenderedAndKicked;
      break;
    case Result::Refined:
      this->_result = Result::RefinedAndKicked;
      break;
    default:
      break;
    }
  }

private:
  Result _result;
};

} // namespace Cesium3DTilesSelection
