#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumUtility/Assert.h>

#include <spdlog/spdlog.h>

#include <memory>
#include <optional>
#include <vector>

namespace Cesium3DTilesSelection {

class GltfModifier::NewVersionLoadRequester : public TileLoadRequester {
public:
  NewVersionLoadRequester(GltfModifier* pModifier);

  double getWeight() const override { return 1.0; }
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
    : _pNewVersionLoadRequester(
          std::make_unique<NewVersionLoadRequester>(this)){};

GltfModifier::~GltfModifier() = default;

std::optional<int> GltfModifier::getCurrentVersion() const {
  return (-1 == this->_currentVersion)
             ? std::nullopt
             : std::optional<int>(this->_currentVersion);
}

void GltfModifier::trigger() {
  ++this->_currentVersion;
  this->_pNewVersionLoadRequester->notifyOfTrigger();
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
  // processing of the new version.
  if (this->_pNewVersionLoadRequester->isRegistered()) {
    this->_pNewVersionLoadRequester->mainThreadQueue.emplace_back(&tile);
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
