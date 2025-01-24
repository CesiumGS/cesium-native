#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>

#include <cstddef>
#include <cstdint>

namespace Cesium3DTilesSelection {

TileOcclusionRendererProxyPool::TileOcclusionRendererProxyPool(
    int32_t maximumPoolSize)
    : _pFreeProxiesHead(nullptr),
      _currentSize(0),
      _maxSize(maximumPoolSize < 0 ? 0 : maximumPoolSize),
      _tileToOcclusionProxyMappings(size_t(this->_maxSize)) {}

TileOcclusionRendererProxyPool::~TileOcclusionRendererProxyPool() {
  this->destroyPool();
}

void TileOcclusionRendererProxyPool::destroyPool() {
  for (auto& pair : this->_tileToOcclusionProxyMappings) {
    this->destroyProxy(pair.second);
  }

  this->_tileToOcclusionProxyMappings.clear();

  while (this->_pFreeProxiesHead) {
    TileOcclusionRendererProxy* pNext = this->_pFreeProxiesHead->_pNext;
    this->destroyProxy(this->_pFreeProxiesHead);
    this->_pFreeProxiesHead = pNext;
  }

  this->_currentSize = 0;
}

const TileOcclusionRendererProxy*
TileOcclusionRendererProxyPool::fetchOcclusionProxyForTile(
    const Tile& tile,
    int32_t /*currentFrame*/) {
  auto mappingIt = this->_tileToOcclusionProxyMappings.find(&tile);
  if (mappingIt != this->_tileToOcclusionProxyMappings.end()) {
    TileOcclusionRendererProxy* pProxy = mappingIt->second;
    pProxy->_usedLastFrame = true;

    return pProxy;
  }

  if (!this->_pFreeProxiesHead && this->_currentSize < this->_maxSize) {
    this->_pFreeProxiesHead = this->createProxy();
    if (this->_pFreeProxiesHead) {
      ++this->_currentSize;
    }
  }

  if (!this->_pFreeProxiesHead) {
    // Pool is full or createProxy returned nullptr
    return nullptr;
  }

  TileOcclusionRendererProxy* pAssignedProxy = this->_pFreeProxiesHead;
  this->_pFreeProxiesHead = this->_pFreeProxiesHead->_pNext;
  pAssignedProxy->_pNext = nullptr;
  pAssignedProxy->_usedLastFrame = true;

  this->_tileToOcclusionProxyMappings.emplace(&tile, pAssignedProxy);
  pAssignedProxy->reset(&tile);

  return pAssignedProxy;
}

void TileOcclusionRendererProxyPool::pruneOcclusionProxyMappings() {
  for (auto iter = this->_tileToOcclusionProxyMappings.begin();
       iter != this->_tileToOcclusionProxyMappings.end();) {
    if (!iter->second->_usedLastFrame) {
      // This tile was not traversed last frame, unmap the proxy and re-add it
      // to the free list.
      iter->second->reset(nullptr);
      iter->second->_pNext = this->_pFreeProxiesHead;
      this->_pFreeProxiesHead = iter->second;
      iter = this->_tileToOcclusionProxyMappings.erase(iter);
    } else {
      // Reset the flag.
      iter->second->_usedLastFrame = false;
      ++iter;
    }
  }
}

} // namespace Cesium3DTilesSelection
