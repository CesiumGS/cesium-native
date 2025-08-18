#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>

#include <spdlog/fwd.h>

#include <atomic>
#include <memory>

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

/** Abstract class that allows modifying a glTF model after it has been loaded.
 * Modifications can include reorganizing the primitives, eg. merging or
 * splitting them. Merging primitives can lead to improved rendering
 * performance. Splitting primitives allows to assign different materials to
 * parts that were initially in the same primitive. The modifier can be applied
 * several times during the lifetime of the model, depending on current needs.
 * Hence the use of a modifier version to be compared to the glTF model version
 * to know whether the mesh is up-to-date, or must be re-processed. A just
 * constructed modifier is considered nilpotent, ie. nothing will happen until
 * trigger() has been called at least once
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

  /** @return The current modifier version, to identify oudated tile models. */
  std::optional<int> getCurrentVersion() const;

  /**
   * @brief Activates this modifier after it has been constructed in its default
   * nilpotent state. Calling it again will apply the modifier again on the
   * already loaded glTF models, by incrementing the version. Tiles already
   * loaded will be re-processed without being unloaded, the new model replacing
   * the old one without transition.
   * See {@link getCurrentVersion}.
   */
  void trigger();

  /**
   * @brief When this modifier has been triggered at least once, this is the
   * method called after a new tile has been loaded, and everytime the
   * modifier's version is incremented with {@link trigger}.
   *
   * This method is called from a worker thread.
   *
   * @param input The input to the glTF modification.
   * @return A future that resolves to a {@link GltfModifierOutput} with the
   * new model, or to `std::nullopt` if the model is not modified.
   */
  virtual CesiumAsync::Future<std::optional<GltfModifierOutput>>
  apply(GltfModifierInput&& input) = 0;

  /**
   * @brief Called by {@link Tileset} when this instance has been registered
   * with it. To add custom behavior on registration, override the other
   * overload of this method.
   * @private
   */
  CesiumAsync::Future<void> onRegister(
      TilesetContentManager& contentManager,
      const TilesetMetadata& tilesetMetadata,
      const Tile& rootTile);

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
   * This method is called from the main thread.
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
  /** The current version of the modifier, if it has ever been triggered.
   * Incremented every time trigger() is called by client code to signal that
   * models needs to be reprocessed.
   */
  std::atomic_int _currentVersion = -1;

  class NewVersionLoadRequester;
  std::unique_ptr<NewVersionLoadRequester> _pNewVersionLoadRequester;
};

} // namespace Cesium3DTilesSelection