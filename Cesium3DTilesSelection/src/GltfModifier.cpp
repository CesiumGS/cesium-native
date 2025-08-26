#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/GltfModifier.h>
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

class GltfModifier::NewVersionLoadRequester : public TileLoadRequester {
public:
  NewVersionLoadRequester(GltfModifier* pModifier);

  double getWeight() const override { return 0.5; }
  bool hasMoreTilesToLoadInWorkerThread() const override;
  const Tile* getNextTileToLoadInWorkerThread() override;
  bool hasMoreTilesToLoadInMainThread() const override;
  const Tile* getNextTileToLoadInMainThread() override;

  void notifyOfTrigger();

  GltfModifier* pModifier;
  const Tile* pRootTile;

  // Ideally these would be weak pointers, but we don't currently have a good
  // way to do that.
  std::vector<Tile::ConstPointer> workerThreadQueue;
  std::vector<Tile::ConstPointer> mainThreadQueue;
};

GltfModifier::GltfModifier()
    : _currentVersion(),
      _pNewVersionLoadRequester(
          std::make_unique<NewVersionLoadRequester>(this)){};

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
    ++*this->_currentVersion;
  }

  this->_pNewVersionLoadRequester->notifyOfTrigger();
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
      GltfModifier::State::WorkerRunning) {
    return false;
  }

  // If modification is WorkerDone, and the version is already up-to-date, we
  // don't need to do it again. But if it's outdated, we want to run the worker
  // thread modification again.
  if (pRenderContent->getGltfModifierState() ==
      GltfModifier::State::WorkerDone) {
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
        pRenderContent->getGltfModifierState() == GltfModifier::State::Idle);
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
  if (pRenderContent->getGltfModifierState() !=
      GltfModifier::State::WorkerDone) {
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
  this->_pNewVersionLoadRequester->pRootTile = &rootTile;
  contentManager.registerTileRequester(*this->_pNewVersionLoadRequester);

  const TilesetExternals& externals = contentManager.getExternals();
  return this->onRegister(
      externals.asyncSystem,
      externals.pAssetAccessor,
      externals.pLogger,
      tilesetMetadata,
      rootTile);
}

void GltfModifier::onUnregister(TilesetContentManager& /* contentManager */) {
  this->_pNewVersionLoadRequester->unregister();
  this->_pNewVersionLoadRequester->mainThreadQueue.clear();
  this->_pNewVersionLoadRequester->workerThreadQueue.clear();
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
    if (this->_pNewVersionLoadRequester->isRegistered()) {
      this->_pNewVersionLoadRequester->workerThreadQueue.emplace_back(&tile);
    }
  }
}

void GltfModifier::onWorkerThreadApplyComplete(const Tile& tile) {
  // GltfModifier::apply just finished, so now we need to do the main-thread
  // processing of the new version. But if the new version is already outdated,
  // we need to do worker thread modification (again) instead of main thread
  // modification.
  if (this->_pNewVersionLoadRequester->isRegistered()) {
    if (tile.needsMainThreadLoading(this)) {
      this->_pNewVersionLoadRequester->mainThreadQueue.emplace_back(&tile);
    } else if (tile.needsWorkerThreadLoading(this)) {
      this->_pNewVersionLoadRequester->workerThreadQueue.emplace_back(&tile);
    }
  }
}

GltfModifier::NewVersionLoadRequester::NewVersionLoadRequester(
    GltfModifier* pModifier_)
    : pModifier(pModifier_),
      pRootTile(nullptr),
      workerThreadQueue(),
      mainThreadQueue() {}

void GltfModifier::NewVersionLoadRequester::notifyOfTrigger() {
  if (!this->isRegistered()) {
    return;
  }

  // Add all already-loaded tiles to this requester worker thread load queue.
  // Tiles that are in ContentLoading will be added to this queue when they
  // finish.
  LoadedConstTileEnumerator enumerator(this->pRootTile);
  for (const Tile& tile : enumerator) {
    TileLoadState state = tile.getState();
    if (state == TileLoadState::ContentLoaded || state == TileLoadState::Done) {
      this->workerThreadQueue.emplace_back(&tile);
    }
  }
}

bool GltfModifier::NewVersionLoadRequester::hasMoreTilesToLoadInWorkerThread()
    const {
  return !this->workerThreadQueue.empty();
}

const Tile*
GltfModifier::NewVersionLoadRequester::getNextTileToLoadInWorkerThread() {
  CESIUM_ASSERT(!this->workerThreadQueue.empty());
  const Tile* pResult = this->workerThreadQueue.back().get();
  this->workerThreadQueue.pop_back();
  return pResult;
}

bool GltfModifier::NewVersionLoadRequester::hasMoreTilesToLoadInMainThread()
    const {
  return !this->mainThreadQueue.empty();
}

const Tile*
GltfModifier::NewVersionLoadRequester::getNextTileToLoadInMainThread() {
  CESIUM_ASSERT(!this->mainThreadQueue.empty());
  const Tile* pResult = this->mainThreadQueue.back().get();
  this->mainThreadQueue.pop_back();
  return pResult;
}

} // namespace Cesium3DTilesSelection
