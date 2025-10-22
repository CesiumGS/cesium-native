#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/GltfModifierState.h>
#include <Cesium3DTilesSelection/GltfModifierVersionExtension.h>
#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Assert.h>

#include <spdlog/logger.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace Cesium3DTilesSelection {

GltfModifier::GltfModifier()
    : _currentVersion(),
      _pRootTile(nullptr),
      _workerThreadQueue(),
      _mainThreadQueue(){};

GltfModifier::~GltfModifier() = default;

std::optional<int64_t> GltfModifier::getCurrentVersion() const {
  return this->_currentVersion;
}

bool GltfModifier::isActive() const {
  return this->getCurrentVersion().has_value();
}

void GltfModifier::trigger() {
  if (!this->_currentVersion) {
    this->_currentVersion = 0;
  } else {
    ++(*this->_currentVersion);
  }

  if (!this->isRegistered()) {
    return;
  }

  // Add all already-loaded tiles to this requester's worker thread load queue.
  // Tiles that are in ContentLoading will be added to this queue when they
  // finish.
  LoadedConstTileEnumerator enumerator(this->_pRootTile);
  for (const Tile& tile : enumerator) {
    if (GltfModifier::needsWorkerThreadModification(this, tile)) {
      this->_workerThreadQueue.emplace_back(&tile);
    }
  }
}

/*static*/ bool GltfModifier::needsWorkerThreadModification(
    const GltfModifier* pModifier,
    const Tile& tile) {
  if (!pModifier)
    return false;

  std::optional<int64_t> modelVersion = pModifier->getCurrentVersion();
  if (!modelVersion)
    return false;

  // If the tile is not loaded at all, there's no need to modify it.
  if (tile.getState() != TileLoadState::Done &&
      tile.getState() != TileLoadState::ContentLoaded) {
    return false;
  }

  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();

  // If a tile has no render content, there's nothing to modify.
  if (!pRenderContent)
    return false;

  // We can't modify a tile for which modification is already in progress.
  if (pRenderContent->getGltfModifierState() ==
      GltfModifierState::WorkerRunning) {
    return false;
  }

  // If modification is WorkerDone, and the version is already up-to-date, we
  // don't need to do it again. But if it's outdated, we want to run the worker
  // thread modification again.
  if (pRenderContent->getGltfModifierState() == GltfModifierState::WorkerDone) {
    const std::optional<CesiumGltf::Model>& maybeModifiedModel =
        pRenderContent->getModifiedModel();
    bool hasUpToDateModifiedModel =
        maybeModifiedModel && GltfModifierVersionExtension::getVersion(
                                  *maybeModifiedModel) == modelVersion;
    return !hasUpToDateModifiedModel;
  } else {
    // Worker is idle. Modification is needed if the model version is out of
    // date.
    CESIUM_ASSERT(
        pRenderContent->getGltfModifierState() == GltfModifierState::Idle);
    bool hasUpToDateModel = GltfModifierVersionExtension::getVersion(
                                pRenderContent->getModel()) == modelVersion;
    return !hasUpToDateModel;
  }
}

/*static*/ bool GltfModifier::needsMainThreadModification(
    const GltfModifier* pModifier,
    const Tile& tile) {
  if (!pModifier)
    return false;

  std::optional<int64_t> modelVersion = pModifier->getCurrentVersion();
  if (!modelVersion)
    return false;

  // Only tiles already Done loading need main thread modification. For
  // ContentLoaded, the modified mesh is applied by the normal transition to
  // Done.
  if (tile.getState() != TileLoadState::Done)
    return false;

  // Only tiles with render content can be modified.
  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();
  if (!pRenderContent)
    return false;

  // We only need to do main thread processing after the worker thread
  // processing has completed.
  if (pRenderContent->getGltfModifierState() != GltfModifierState::WorkerDone) {
    return false;
  }

  // We only need to do main thread processing if there's a modified model.
  const std::optional<CesiumGltf::Model>& maybeModifiedModel =
      pRenderContent->getModifiedModel();
  if (!maybeModifiedModel)
    return false;

  // If the version of the modified model is wrong (outdated), there's no point
  // in doing main thread processing. We need to do worker thread processing
  // again first.
  if (GltfModifierVersionExtension::getVersion(*maybeModifiedModel) !=
      modelVersion) {
    return false;
  }

  return true;
}

CesiumAsync::Future<void> GltfModifier::onRegister(
    TilesetContentManager& contentManager,
    const TilesetMetadata& tilesetMetadata,
    const Tile& rootTile) {
  this->_pRootTile = &rootTile;
  contentManager.registerTileRequester(*this);

  const TilesetExternals& externals = contentManager.getExternals();
  return this->onRegister(
      externals.asyncSystem,
      externals.pAssetAccessor,
      externals.pLogger,
      tilesetMetadata,
      rootTile);
}

void GltfModifier::onUnregister(TilesetContentManager& /* contentManager */) {
  this->unregister();
  this->_mainThreadQueue.clear();
  this->_workerThreadQueue.clear();
}

CesiumAsync::Future<void> GltfModifier::onRegister(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& /* pAssetAccessor */,
    const std::shared_ptr<spdlog::logger>& /* pLogger */,
    const TilesetMetadata& /* tilesetMetadata */,
    const Tile& /* rootTile */) {
  return asyncSystem.createResolvedFuture();
}

void GltfModifier::onOldVersionContentLoadingComplete(const Tile& tile) {
  CESIUM_ASSERT(
      tile.getState() == TileLoadState::ContentLoaded ||
      tile.getState() == TileLoadState::Failed ||
      tile.getState() == TileLoadState::FailedTemporarily);
  if (tile.getState() == TileLoadState::ContentLoaded) {
    // Tile just transitioned from ContentLoading -> ContentLoaded, but it did
    // so based on the load version. Add it to the worker thread queue in order
    // to re-run the GltfModifier on it.
    if (this->isRegistered() &&
        GltfModifier::needsWorkerThreadModification(this, tile)) {
      this->_workerThreadQueue.emplace_back(&tile);
    }
  }
}

void GltfModifier::onWorkerThreadApplyComplete(const Tile& tile) {
  // GltfModifier::apply just finished, so now we need to do the main-thread
  // processing of the new version. But if the new version is already outdated,
  // we need to do worker thread modification (again) instead of main thread
  // modification.
  if (this->isRegistered()) {
    if (tile.needsMainThreadLoading(this)) {
      this->_mainThreadQueue.emplace_back(&tile);
    } else if (tile.needsWorkerThreadLoading(this)) {
      this->_workerThreadQueue.emplace_back(&tile);
    }
  }
}

double GltfModifier::getWeight() const { return 0.5; }

bool GltfModifier::hasMoreTilesToLoadInWorkerThread() const {
  return !this->_workerThreadQueue.empty();
}

const Tile* GltfModifier::getNextTileToLoadInWorkerThread() {
  CESIUM_ASSERT(!this->_workerThreadQueue.empty());
  const Tile* pResult = this->_workerThreadQueue.back().get();
  this->_workerThreadQueue.pop_back();
  return pResult;
}

bool GltfModifier::hasMoreTilesToLoadInMainThread() const {
  return !this->_mainThreadQueue.empty();
}

const Tile* GltfModifier::getNextTileToLoadInMainThread() {
  CESIUM_ASSERT(!this->_mainThreadQueue.empty());
  const Tile* pResult = this->_mainThreadQueue.back().get();
  this->_mainThreadQueue.pop_back();
  return pResult;
}

} // namespace Cesium3DTilesSelection
