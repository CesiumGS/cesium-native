#pragma once

#include <Cesium3DTilesSelection/Library.h>

#include <cstdint>

namespace Cesium3DTilesSelection {

/**
 * @brief A description of the state of a {@link Tile} during the rendering
 * process
 *
 * Instances of this class combine a frame number and a
 * {@link TileSelectionState::Result} that describes the actual state of the
 * tile.
 * Instances of this class are stored in a {@link Tile}, and are used to track
 * the state of the tile during the rendering process. The {@link Tileset}
 * updates this state while traversing the tile hierarchy, tracking whether a
 * tile was rendered, culled, or refined in the last frame.
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
     * list in favor of an ancestor because it is not yet renderable.
     */
    RenderedAndKicked = 4,

    /**
     * @brief This tile was refined but then removed from the render list
     *
     * This tile was originally refined, but its rendered descendants got kicked
     * out of the render list in favor of an ancestor because it is not yet
     * renderable.
     */
    RefinedAndKicked = 5
  };

  /**
   * @brief Initializes a new instance with
   * {@link TileSelectionState::Result::None}
   */
  constexpr TileSelectionState() noexcept
      : _frameNumber(0), _result(Result::None) {}

  /**
   * @brief Initializes a new instance with a given
   * {@link TileSelectionState::Result}.
   *
   * @param frameNumber The frame number in which the selection took place.
   * @param result The result of the selection.
   */
  constexpr TileSelectionState(int32_t frameNumber, Result result) noexcept
      : _frameNumber(frameNumber), _result(result) {}

  /**
   * @brief Gets the frame number in which selection took place.
   */
  constexpr int32_t getFrameNumber() const noexcept {
    return this->_frameNumber;
  }

  /**
   * @brief Gets the result of selection.
   *
   * The given frame number must match the frame number in which selection last
   * took place. Otherwise, {@link TileSelectionState::Result::None} is
   * returned.
   *
   * @param frameNumber The previous frame number.
   * @return The {@link TileSelectionState::Result}
   */
  constexpr Result getResult(int32_t frameNumber) const noexcept {
    if (this->_frameNumber != frameNumber) {
      return Result::None;
    }
    return this->_result;
  }

  /**
   * @brief Determines if this tile or its descendents were kicked from the
   * render list.
   *
   * In other words, if its last selection result was
   * {@link TileSelectionState::Result::RenderedAndKicked} or
   * {@link TileSelectionState::Result::RefinedAndKicked}.
   *
   * @param frameNumber The previous frame number.
   * @return `true` if the tile was kicked, and `false` otherwise
   */
  constexpr bool wasKicked(int32_t frameNumber) const noexcept {
    const Result result = this->getResult(frameNumber);
    return result == Result::RenderedAndKicked ||
           result == Result::RefinedAndKicked;
  }

  /**
   * @brief Gets the original selection result prior to being kicked.
   *
   * If the tile wasn't kicked, the original value is returned.
   *
   * @param frameNumber The previous frame number.
   * @return The {@link TileSelectionState::Result} prior to being kicked.
   */
  constexpr Result getOriginalResult(int32_t frameNumber) const noexcept {
    const Result result = this->getResult(frameNumber);

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
  int32_t _frameNumber;
  Result _result;
};

} // namespace Cesium3DTilesSelection
