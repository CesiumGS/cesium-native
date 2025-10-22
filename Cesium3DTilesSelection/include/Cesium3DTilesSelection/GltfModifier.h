#pragma once

#include <Cesium3DTilesSelection/GltfModifierState.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>

#include <spdlog/fwd.h>

#include <memory>
#include <optional>

namespace CesiumAsync {

class IAssetAccessor;

} // namespace CesiumAsync

namespace Cesium3DTilesSelection {

class TilesetMetadata;
class TilesetContentManager;

/**
 * @brief The input to @ref GltfModifier::apply.
 */
struct GltfModifierInput {
  /**
   * @brief The version of the @ref GltfModifier, as returned by
   * @ref GltfModifier::getCurrentVersion at the start of the modification.
   *
   * This is provided because calling @ref GltfModifier::getCurrentVersion
   * may return a newer version if @ref GltfModifier::trigger is called
   * again while @ref GltfModifier::apply is running in a worker thread.
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
 * @brief The output of @ref GltfModifier::apply.
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
 * primitives allows different materials to be assigned to parts that were
 * initially in the same primitive.
 *
 * The `GltfModifier` can be applied several times during the lifetime of the
 * model, depending on current needs. For this reason, the `GltfModifer` has a
 * @ref getCurrentVersion, which can be incremented by calling
 * @ref trigger. When the version is incremented, the `GltfModifier` will be
 * re-applied to all previously-modified models.
 *
 * The version number of a modified glTF is stored in the
 * @ref GltfModifierVersionExtension extension.
 *
 * A just-constructed modifier is considered nilpotent, meaning nothing will
 * happen until @ref trigger has been called at least once.
 *
 * The @ref apply function is called from a worker thread. All other methods
 * must only be called from the main thread.
 */
class GltfModifier : private TileLoadRequester {
public:
  /**
   * @brief Gets the current version number, or `std::nullopt` if the
   * `GltfModifier` is currently inactive.
   *
   * Returns `std::nullopt` when in the default nilpotent state where glTFs will
   * not be modified at all. Calling @ref trigger once will set the version
   * number to 0 and activate the `GltfModifier`. Calling it successive times
   * will increment the version number and re-apply modification to all
   * previously-modified models.
   */
  std::optional<int64_t> getCurrentVersion() const;

  /**
   * @brief Checks if this `GltfModifier` is active.
   *
   * This method returns true if the current version is greater than or equal to
   * 0, indicating that @ref trigger has been called at least once.
   */
  bool isActive() const;

  /**
   * @brief Call this the first time to activate this `GltfModifier` after it
   * has been constructed in its default nilpotent state and set the
   * @ref getCurrentVersion to 0. Call it successive times to increment
   * @ref getCurrentVersion and reapply modification to all
   * previously-modified models without unloading them.
   *
   * While the `GltfModifier` is being reapplied for a new version, the display
   * may show a mix of tiles with the old and new versions.
   */
  void trigger();

  /**
   * @brief Implement this method to apply custom modification to a glTF model.
   * It is called by the @ref Tileset from within a worker thread.
   *
   * This method will be called for each @ref Tile during the content load
   * process if @ref trigger has been called at least once. It will also be
   * called again for already-loaded tiles for successive calls to
   * @ref trigger.
   *
   * @param input The input to the glTF modification.
   * @return A future that resolves to a @ref GltfModifierOutput with the
   * new model, or to `std::nullopt` if the model does not need to be modified.
   */
  virtual CesiumAsync::Future<std::optional<GltfModifierOutput>>
  apply(GltfModifierInput&& input) = 0;

  /**
   * @brief Checks if the given tile needs to be processed by this
   * `GltfModifier` in a worker thread.
   *
   * @param tile The tile to check.
   * @returns `true` if the tile needs to be processed by the `GltfModifier` in
   * a worker thread, or `false` otherwise.
   */
  bool needsWorkerThreadModification(const Tile& tile) const;

  /**
   * @brief Checks if the given tile needs to be processed by this
   * `GltfModifier` in the main thread.

   * @param tile The tile to check.
   * @returns `true` if the tile needs to be processed by the `GltfModifier` in
   * the main thread, or `false` otherwise.
   */
  bool needsMainThreadModification(const Tile& tile) const;

protected:
  GltfModifier();
  virtual ~GltfModifier();

  /**
   * @brief Notifies this instance that it has been registered with a
   * @ref Tileset.
   *
   * This method is called after the tileset's root tile is known but
   * before @ref Tileset::getRootTileAvailableEvent has been raised.
   *
   * This method is called from the main thread. Override this method to respond
   * to this event.
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
   * @brief Called by @ref Tileset when this instance has been registered
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
   * @brief Called by @ref Tileset when this instance has been unregistered
   * from it.
   */
  void onUnregister(TilesetContentManager& contentManager);

  /**
   * @private
   *
   * @brief Called by @ref Tileset when the given tile leaves the
   * @ref TileLoadState::ContentLoading state but it was loaded with an
   * older @ref GltfModifier version. The tile will be queued for a call
   * to @ref apply in a worker thread.
   *
   * This method is called from the main thread.
   *
   * @param tile The tile that has just left the
   * @ref TileLoadState::ContentLoading state.
   */
  void onOldVersionContentLoadingComplete(const Tile& tile);

  /**
   * @private
   *
   * @brief Called by @ref Tileset when @ref apply has finished running on a
   * previously-loaded tile. The tile will be queued to finish its loading in
   * the main thread.
   *
   * This method is called from the main thread.
   *
   * @param tile The tile that has just been processed by @ref apply.
   */
  void onWorkerThreadApplyComplete(const Tile& tile);

  // TileLoadRequester implementation
  double getWeight() const override;
  bool hasMoreTilesToLoadInWorkerThread() const override;
  const Tile* getNextTileToLoadInWorkerThread() override;
  bool hasMoreTilesToLoadInMainThread() const override;
  const Tile* getNextTileToLoadInMainThread() override;

  std::optional<int64_t> _currentVersion;

  const Tile* _pRootTile;

  // Ideally these would be weak pointers, but we don't currently have a good
  // way to do that.
  std::vector<Tile::ConstPointer> _workerThreadQueue;
  std::vector<Tile::ConstPointer> _mainThreadQueue;

  friend class TilesetContentManager;
  friend class MockTilesetContentManagerForGltfModifier;
};

} // namespace Cesium3DTilesSelection
