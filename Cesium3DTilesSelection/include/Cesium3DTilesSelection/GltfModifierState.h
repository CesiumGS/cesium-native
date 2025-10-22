#pragma once

namespace Cesium3DTilesSelection {

/** The state of the @ref GltfModifier process for a given tile content's model.
 */
enum class GltfModifierState {
  /** Modifier is not currently processing this tile. */
  Idle,

  /** Worker-thread phase is in progress. */
  WorkerRunning,

  /**
   * The worker-thread phase is complete, but the main-thread phase is not done
   * yet. When the main thread phase eventually runs, it will call @ref
   * TileRenderContent::replaceWithModifiedModel and then transition the @ref
   * GltfModifier back to the `Idle` state.
   */
  WorkerDone,
};

} // namespace Cesium3DTilesSelection
