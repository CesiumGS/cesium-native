#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <Cesium3DTilesSelection/spdlog-cesium.h>
#include <rapidjson/fwd.h>
#include <CesiumAsync/AsyncSystem.h>

#include <glm/fwd.hpp>

#include <atomic>
#include <memory>

namespace CesiumAsync {
class IAssetAccessor;
class ITaskProcessor;
} // namespace CesiumAsync

namespace CesiumUtility {
class CreditSystem;
}

namespace Cesium3DTilesSelection {
class IPrepareRendererResources;

/** Abstract class that allows tuning a glTF model.
 * "Tuning" means reorganizing the primitives, eg. merging or splitting them.
 * Merging primitives can lead to improved rendering performance.
 * Splitting primitives allows to assign different materials to parts that were
 * initially in the same primitive. Tuning is done in 2 phases: first phase in
 * worker thread, then second phase in main thread. Tuning can occur several
 * times during the lifetime of the model, depending on current needs. Hence
 * the use of a "tune version" which allows to know if the mesh is up-to-date,
 * or must be re-processed.
 * A just constructed tuner is considered nilpotent, ie. tuning
 * will not happen until retune() has been called at least once
 */
class GltfTuner
{
public:
  static constexpr int initialVersion = -1;

private:
  /** The current version of the tuner, which should be incremented by client
   * code whenever models needs to be re-tuned.
   * @see initialVersion
   */
  std::atomic_int currentVersion = initialVersion;

public:
  virtual ~GltfTuner() = default;

  /** @return The current tuner version, to identify oudated tile models. */
  int getCurrentVersion() const { return currentVersion; }

  /** Increment the tuner version, which is also required to activate the tuner
   * after it has been constructed in its default nilpotent state. Tiles already
   * loaded will be re-processed without being unloaded, the new model replacing
   * the old one without transition. */
  int retune() { return ++currentVersion; }

  /** The method called after a new tile has been loaded, and everytime the
   * tuner's version is incremented with {@link retune}.
   * @param model Input model that may have to be processed
   * @param tileTransform Transformation of the model's tile.
   *     See {@link Cesium3DTilesSelection::Tile::getTransform}.
   * @param rootTranslation Translation of the root tile of the tileset
   * @param tunedModel Target of the transformation process. My be equal to the
   * input model.
   * @return True if any processing was done and the result placed in the
   * tunedModel parameter, false when no processing was needed, in which case
   * the tunedModel parameter was ignored.
   */
  virtual bool apply(
      const CesiumGltf::Model& model,
      const glm::dmat4& tileTransform,
      const glm::dvec4& rootTranslation,
      CesiumGltf::Model& tunedModel) = 0;

  /** Called during a tileset's initialization process to let the tuner get
   * extra information from the tileset metadata before any tile has been
   * loaded. */
  virtual void parseTilesetJson(const rapidjson::Document& tilesetJson) = 0;
};

/**
 * @brief External interfaces used by a {@link Tileset}.
 *
 * Not supposed to be used by clients.
 */
class CESIUM3DTILESSELECTION_API TilesetExternals final {
public:
  /**
   * @brief An external {@link CesiumAsync::IAssetAccessor}.
   */
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  /**
   * @brief An external {@link IPrepareRendererResources}.
   */
  std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources;

  /**
   * @brief The async system to use to do work in threads.
   *
   * The tileset will automatically call
   * {@link CesiumAsync::AsyncSystem::dispatchMainThreadTasks} from
   * {@link Tileset::updateView}.
   */
  CesiumAsync::AsyncSystem asyncSystem;

  /**
   * @brief An external {@link CesiumUtility::CreditSystem} that can be used to manage credit
   * strings and periodically query which credits to show and and which to
   * remove from the screen.
   */
  std::shared_ptr<CesiumUtility::CreditSystem> pCreditSystem;

  /**
   * @brief A spdlog logger that will receive log messages.
   *
   * If not specified, defaults to `spdlog::default_logger()`.
   */
  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();

  /**
   * @brief A pool of renderer proxies to determine the occlusion state of
   * tile bounding volumes.
   *
   * If not specified, the traversal will not attempt to leverage occlusion
   * information.
   */
  std::shared_ptr<TileOcclusionRendererProxyPool> pTileOcclusionProxyPool =
      nullptr;

  /**
   * @brief The shared asset system used to facilitate sharing of common assets,
   * such as images, between and within tilesets.
   */
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> pSharedAssetSystem =
      TilesetSharedAssetSystem::getDefault();

  /**
   * Optional user-controlled tile loading post-processing stage that can modify
   * the glTF meshes (eg. split or merge them).
   *
   * @see Cesium3DTilesSelection::GltfTuner
   */
  std::shared_ptr<GltfTuner> gltfTuner;
};

} // namespace Cesium3DTilesSelection
