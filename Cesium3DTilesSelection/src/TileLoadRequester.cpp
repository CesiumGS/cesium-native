#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <CesiumUtility/Assert.h>

#include <utility>

namespace Cesium3DTilesSelection {

void TileLoadRequester::unregister() noexcept {
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->unregisterTileRequester(*this);
    this->_pTilesetContentManager.reset();
  }
}

TileLoadRequester::TileLoadRequester() noexcept
    : _pTilesetContentManager(nullptr) {}

TileLoadRequester::TileLoadRequester(const TileLoadRequester& /*rhs*/) noexcept
    : _pTilesetContentManager(nullptr) {
  // A copy is not automatically registered with the content manager.
}

TileLoadRequester::TileLoadRequester(TileLoadRequester&& rhs) noexcept
    : _pTilesetContentManager(std::move(rhs._pTilesetContentManager)) {
  CESIUM_ASSERT(rhs._pTilesetContentManager == nullptr);
  if (this->_pTilesetContentManager) {
    this->_pTilesetContentManager->registerTileRequester(*this);
    this->_pTilesetContentManager->unregisterTileRequester(rhs);
  }
}

TileLoadRequester::~TileLoadRequester() noexcept { this->unregister(); }

} // namespace Cesium3DTilesSelection
