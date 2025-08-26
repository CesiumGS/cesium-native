#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>

#include <spdlog/fwd.h>

#include <atomic>
#include <memory>
#include <optional>

namespace CesiumAsync {

class IAssetAccessor;

} // namespace CesiumAsync

namespace Cesium3DTilesSelection {

class Tile;
class TilesetMetadata;
class TilesetContentManager;

/**
 * @brief The input to the {@link GltfModifier::apply} function.
 */
struct GltfModifierInput {
  /**
   * @brief The {@link GltfModifier}'s version, as returned by
   * {@link GltfModifier::getCurrentVersion} at the start of the modification.
   *
   * This is provided because calling {@link GltfModifier::getCurrentVersion}
   * may return a newer version if {@link GltfModifier::trigger} is called
   * again while {@link GltfModifier::apply} is running in a worker thread.
   */
  int64_t version;

  /**
   * @brief The async system that can be used to do work in threads.
   */
  CesiumAsync::AsyncSystem asyncSystem;

  /**
   * @brief An asset accessor that can be used to obtain additional assets.
   */
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  /**
   * @brief The logger.
   */
  std::shared_ptr<spdlog::logger> pLogger;

  /**
   * @brief The model to be modified.
   */
  const CesiumGltf::Model& previousModel;

  /**
   * @brief The transformation of the model's coordinates to the tileset's
   * coordinate system.
   */
  glm::dmat4 tileTransform;
};

/**
 * @brief The output of the {@link GltfModifier::apply} function.
 */
struct GltfModifierOutput {
  /**
   * @brief The new, modified model.
   */
  CesiumGltf::Model modifiedModel;
};

/**
 * @brief An abstract class that allows modifying a tile's glTF model after it
 * has been loaded.
 *
 * An example modification is merging or splitting the primitives in the glTF.
 * Merging primitives can lead to improved rendering performance. Splitting
 * primitives allows to assign different materials to parts that were initially
 * in the same primitive.
 *
 * The `GltfModifier` can be applied several times during the lifetime of the
 * model, depending on current needs. For this reason, the `GltfModifer` has a
 * {@link getCurrentVersion}, which can be incremented by calling
 * {@link trigger}. When the version is incremented, the `GltfModifier` will be
 * re-applied to all previously-modified models.
 *
 * The version number of a modified glTF is stored in the
 * {@link GltfModifierVersionExtension} extension.
 *
 * A just-constructed modifier is considered nilpotent, meaning nothing will
 * happen until {@link trigger} has been called at least once.
 *
 * The {@link apply} function is called from a worker thread. All other methods
 * must only be called from the main thread.
 */
class GltfModifier {
public:
  /** The state of the glTF modifier process for a given tile content's model.
   */
  enum class State {
    /** Modifier is not currently processing this tile. */
    Idle,
    /** Worker-thread phase is in progress. */
    WorkerRunning,
    /** Worker-thread phase is complete, main-thread phase is not done yet. */
    WorkerDone,
  };

  /**
   * @brief Gets the current version number, or `std::nullopt` if the
   * `GltfModifier` is currently inactive.
   *
   * Returns `std::nullopt` when in the default nilpotent state where glTFs will
   * not be modified at all. Call {@link trigger} once to set the version
   * number to 0 and activate the `GltfModifier`. Call it successive times to
   * increment the version number and re-apply modification to all
   * previously-modified models.
   */
  std::optional<int64_t> getCurrentVersion() const;

  /**
   * @brief Checks if this `GltfModifier` is active.
   *
   * This method returns true if the current version is greater than or equal to
   * 0, indicating that {@link trigger} has been called at least once.
   */
  bool isActive() const;

  /**
   * @brief Call this the first time to activate this `GltfModifier` after it
   * has been constructed in its default nilpotent state and set the
   * {@link getCurrentVersion} to 0. Call it successive times to increment
   * {@link getCurrentVersion} and reapply modification to all
   * previously-modified models without unloading them.
   *
   * While the `GltfModifier` is being reapplied for a new version, the display
   * may show a mix of tiles with the old and new versions.
   */
  void trigger();

  /**
   * @brief Implement this method to apply custom modification to a glTF model.
   * It is called by the {@link Tileset} from within a worker thread.
   *
   * This method will be called for each {@link Tile} during the content load
   * process if {@link trigger} has been called at least once. It will also be
   * called again for already-loaded tiles for successive calls to
   * {@link trigger}.
   *
   * @param input The input to the glTF modification.
   * @return A future that resolves to a {@link GltfModifierOutput} with the
   * new model, or to `std::nullopt` if the model does not need to be modified.
   */
  virtual CesiumAsync::Future<std::optional<GltfModifierOutput>>
  apply(GltfModifierInput&& input) = 0;

  /**
   * @brief Checks if the given tile needs to be processed by the given
   * `GltfModifier` in a worker thread.
   * @param pModifier The `GltfModifier` to check.
   * @param tile The tile to check.
   * @returns `true` if the tile needs to be processed by the `GltfModifier` in
   * a worker thread, or `false` otherwise.
   */
  static bool needsWorkerThreadModification(
      const GltfModifier* pModifier,
      const Tile& tile);

  /**
   * @brief Checks if the given tile needs to be processed by the given
   * `GltfModifier` in the main thread.
   * @param pModifier The `GltfModifier` to check.
   * @param tile The tile to check.
   * @returns `true` if the tile needs to be processed by the `GltfModifier` in
   * the main thread, or `false` otherwise.
   */
  static bool
  needsMainThreadModification(const GltfModifier* pModifier, const Tile& tile);

protected:
  GltfModifier();
  virtual ~GltfModifier();

  /**
   * @brief Notifies this instance that is has been registered with a
   * {@link Tileset}.
   *
   * This method is called after the tileset's root tile is known but
   * before {@link Tileset::getRootTileAvailableEvent} has been raised.
   *
   * This method is called from the main thread. Override this method respond to
   * this event.
   *
   * @param asyncSystem The async system with which to do background work.
   * @param pAssetAccessor The asset accessor to use to retrieve any additional
   * assets.
   * @param pLogger The logger to which to log errors and warnings that occur
   * during preparation of the `GltfModifier`.
   * @param tilesetMetadata The metadata associated with the tileset.
   * @param rootTile The root tile of the tileset.
   * @returns A future that resolves when the `GltfModifier` is ready to modify
   * glTF instances for this tileset. Tileset loading will not proceed until
   * this future resolves. If the future rejects, tileset load will proceed but
   * the `GltfModifier` will not be used.
   */
  virtual CesiumAsync::Future<void> onRegister(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const TilesetMetadata& tilesetMetadata,
      const Tile& rootTile);

private:
  /**
   * @private
   *
   * @brief Called by {@link Tileset} when this instance has been registered
   * with it. To add custom behavior on registration, override the other
   * overload of this method.
   */
  CesiumAsync::Future<void> onRegister(
      TilesetContentManager& contentManager,
      const TilesetMetadata& tilesetMetadata,
      const Tile& rootTile);

  /**
   * @private
   *
   * @brief Called by {@link Tileset} when this instance has been unregistered
   * from it.
   */
  void onUnregister(TilesetContentManager& contentManager);

  /**
   * @private
   *
   * @brief Called by {@link Tileset} when the given tile leaves the
   * {@link TileLoadState::ContentLoading} state but it was loaded with an
   * older {@link GltfModifier} version. The tile will be queued for a call
   * to {@link GltfModifier::apply} in a worker thread.
   *
   * This method is called from the main thread.
   *
   * @param tile The tile that has just left the
   * {@link TileLoadState::ContentLoading} state.
   */
  void onOldVersionContentLoadingComplete(const Tile& tile);

  /**
   * @private
   *
   * @brief Called by {@link Tileset} when the {@link GltfModifier::apply}
   * method has finished running on a previously-loaded tile. The tile will be
   * queued to finish its loading in the main thread.
   *
   * This method is called from the main thread.
   *
   * @param tile The tile that has just been processed by the
   * {@link GltfModifier::apply} method.
   */
  void onWorkerThreadApplyComplete(const Tile& tile);

  std::optional<int64_t> _currentVersion;

  class NewVersionLoadRequester;
  std::unique_ptr<NewVersionLoadRequester> _pNewVersionLoadRequester;

  friend class TilesetContentManager;
};

} // namespace Cesium3DTilesSelection