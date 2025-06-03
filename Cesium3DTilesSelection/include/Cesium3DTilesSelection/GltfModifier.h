#pragma once

#include <CesiumGltf/Model.h>

#include <atomic>
#include <memory>

namespace Cesium3DTilesSelection {

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
  /** The current version of the modifier, if it has ever been triggered.
   * Incremented every time trigger() is called by client code to signal that
   * models needs to be reprocessed.
   */
  std::atomic_int currentVersion = -1;

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

public:
  virtual ~GltfModifier() = default;

  /** @return The current modifier version, to identify oudated tile models. */
  std::optional<int> getCurrentVersion() const {
    return (-1 == currentVersion) ? std::nullopt
                                  : std::optional<int>(currentVersion);
  }

  /** Activates this modifier after it has been constructed in its default
   * nilpotent state. Calling it again will apply the modifier again on the
   * already loaded glTF models, by incrementing the version. Tiles already
   * loaded will be re-processed without being unloaded, the new model replacing
   * the old one without transition.
   * See {@link getCurrentVersion}.
   */
  void trigger() { ++currentVersion; }

  /** When this modifier has been triggered at least once, this is the method
   * called after a new tile has been loaded, and everytime the
   * modifier's version is incremented with {@link trigger}.
   * @param model Input model that may have to be processed
   * @param tileTransform Transformation of the model's tile.
   *     See {@link Cesium3DTilesSelection::Tile::getTransform}.
   * @param rootTranslation Translation of the root tile of the tileset
   * @param modifiedModel Target of the transformation process. May be equal to
   * the input model.
   * @return True if any processing was done and the result placed in the
   * modifiedModel parameter, false when no processing was needed, in which case
   * the modifiedModel parameter was ignored.
   */
  virtual bool apply(
      const CesiumGltf::Model& model,
      const glm::dmat4& tileTransform,
      const glm::dvec4& rootTranslation,
      CesiumGltf::Model& modifiedModel) = 0;

  /** Called during a tileset's initialization process to let the modifier get
   * extra information from the tileset metadata before any tile has been
   * loaded. */
  virtual void parseTilesetJson(const rapidjson::Document& tilesetJson) = 0;
};

} // namespace Cesium3DTilesSelection