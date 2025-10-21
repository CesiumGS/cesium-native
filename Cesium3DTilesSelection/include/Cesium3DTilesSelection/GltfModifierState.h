#pragma once

namespace Cesium3DTilesSelection {

/** The state of the @ref GltfModifier process for a given tile content's model.
 */
enum class GltfModifierState {
  /** Modifier is not currently processing this tile. */
  Idle,
  /** Worker-thread phase is in progress. */
  WorkerRunning,
  /** Worker-thread phase is complete, main-thread phase is not done yet. */
  WorkerDone,
};

} // namespace Cesium3DTilesSelection
