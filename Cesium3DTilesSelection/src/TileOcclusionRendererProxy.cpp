#include "Cesium3DTilesSelection/TileOcclusionRendererProxy.h"

namespace Cesium3DTilesSelection {

TileOcclusionRendererProxyPool::~TileOcclusionRendererProxyPool() {
  this->destroyPool();
}

void TileOcclusionRendererProxyPool::initPool(uint32_t poolSize) {
  this->_tileToOcclusionProxyMappings.reserve(poolSize);

  TileOcclusionRendererProxy* pLastProxy = nullptr;
  for (uint32_t i = 0; i < poolSize; ++i) {
    TileOcclusionRendererProxy* pCurrentProxy = this->createProxy();
    if (pCurrentProxy) {
      pCurrentProxy->_pNext = pLastProxy;
      pLastProxy = pCurrentProxy;
    }
  }

  this->_pFreeProxiesHead = pLastProxy;
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

  if (!this->_pFreeProxiesHead) {
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
