#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>

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

private:
  void addTilesToQueues();

  GltfModifier* _pModifier;
  const Tile* _pRootTile;
  std::vector<Tile::ConstPointer> _workerThreadQueue;
  std::vector<Tile::ConstPointer> _mainThreadQueue;
  bool _running;
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
  contentManager.registerTileRequester(*this->_pNewVersionLoadRequester);

  const TilesetExternals& externals = contentManager.getExternals();
  return this->onRegister(
      externals.asyncSystem,
      externals.pAssetAccessor,
      externals.pLogger,
      tilesetMetadata,
      rootTile);
}

CesiumAsync::Future<void> GltfModifier::onRegister(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& /* pAssetAccessor */,
    const std::shared_ptr<spdlog::logger>& /* pLogger */,
    const TilesetMetadata& /* tilesetMetadata */,
    const Tile& /* rootTile */) {
  return asyncSystem.createResolvedFuture();
}

GltfModifier::NewVersionLoadRequester::NewVersionLoadRequester(
    GltfModifier* pModifier)
    : _pModifier(pModifier),
      _pRootTile(nullptr),
      _workerThreadQueue(),
      _mainThreadQueue(),
      _running(false) {}

void GltfModifier::NewVersionLoadRequester::notifyOfTrigger() {
  this->_running = true;
  this->addTilesToQueues();
}

void GltfModifier::NewVersionLoadRequester::addTilesToQueues() {
  CESIUM_ASSERT(this->_pRootTile != nullptr);
  LoadedConstTileEnumerator enumerator(this->_pRootTile);
  for (const Tile& tile : enumerator) {
    TileLoadState state = tile.getState();

    // Tiles that are already being loaded will need to finish loading, _then_
    // we'll have to apply the modification. Always add them to the queue.
    if (state == TileLoadState::ContentLoading) {
      this->_workerThreadQueue.emplace_back(&tile);
    }

    // Tiles that are already loaded only need to be added to the queue if they
    // have a glTF and it is the wrong version.
    if (state != TileLoadState::ContentLoaded && state != TileLoadState::Done)
      continue;

    const TileRenderContent* pRenderContent =
        tile.getContent().getRenderContent();
    if (!pRenderContent)
      continue;

    if (pRenderContent->getModel().version ==
        this->_pModifier->getCurrentVersion()) {
      CESIUM_ASSERT(!pRenderContent->getModifiedModel());
      continue;
    }

    if (pRenderContent->getModifiedModel() &&
        pRenderContent->getModifiedModel()->version ==
            this->_pModifier->getCurrentVersion()) {
      // Worker thread GltfModifier::apply done, need to do main thread loading.
      this->_mainThreadQueue.emplace_back(&tile);
    } else {
      this->_workerThreadQueue.emplace_back(&tile);
    }
  }
}

bool GltfModifier::NewVersionLoadRequester::hasMoreTilesToLoadInWorkerThread()
    const {
  return !this->_workerThreadQueue.empty();
}

const Tile*
GltfModifier::NewVersionLoadRequester::getNextTileToLoadInWorkerThread() {
  if (!this->_running) {
    CESIUM_ASSERT(this->_workerThreadQueue.empty());
    return nullptr;
  }

  CESIUM_ASSERT(!this->_workerThreadQueue.empty());
  const Tile* pResult = this->_workerThreadQueue.back().get();
  this->_workerThreadQueue.pop_back();

  if (this->_workerThreadQueue.empty()) {
    this->addTilesToQueues();

    // If both queues are still empty, we're done.
    if (this->_workerThreadQueue.empty() && this->_mainThreadQueue.empty()) {
      this->_running = false;
    }
  }

  return pResult;
}

bool GltfModifier::NewVersionLoadRequester::hasMoreTilesToLoadInMainThread()
    const {
  return !this->_mainThreadQueue.empty();
}

const Tile*
GltfModifier::NewVersionLoadRequester::getNextTileToLoadInMainThread() {
  if (!this->_running) {
    CESIUM_ASSERT(this->_mainThreadQueue.empty());
    return nullptr;
  }

  CESIUM_ASSERT(!this->_mainThreadQueue.empty());
  const Tile* pResult = this->_mainThreadQueue.back().get();
  this->_mainThreadQueue.pop_back();

  if (this->_mainThreadQueue.empty()) {
    this->addTilesToQueues();

    // If both queues are still empty, we're done.
    if (this->_workerThreadQueue.empty() && this->_mainThreadQueue.empty()) {
      this->_running = false;
    }
  }

  return pResult;
}

} // namespace Cesium3DTilesSelection
