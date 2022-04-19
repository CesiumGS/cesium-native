#include "Cesium3DTilesSelection/TileOcclusionRendererProxy.h"

namespace Cesium3DTilesSelection {

void TileOcclusionRendererProxy::update(int32_t currentFrame) {
  this->update(currentFrame);
  this->_usedLastFrame = true;
}

void TileOcclusionRendererProxyPool::initPool(int32_t poolSize) {
  this->_tileToOcclusionProxyMapping.reserve(poolSize);

  TileOcclusionRendererProxy* pLastProxy = nullptr;
  for (int32_t i = 0; i < poolSize; ++i) {
    TileOcclusionRendererProxy* pCurrentProxy = this->createProxy();
    if (pCurrentProxy) {
      pCurrentProxy->_pNext = pLastProxy;
      pLastProxy = pCurrentProxy;
    }
  }

  this->_pFreeProxiesHead = pLastProxy;
}

void TileOcclusionRendererProxyPool::destroyPool() {
  for (auto& pair : this->_tileToOcclusionProxyMapping) {
    this->destroyProxy(pair.second);
  }

  this->_tileToOcclusionProxyMapping.clear();

  TileOcclusionRendererProxy* pCurrent = this->_pFreeProxiesHead;
  while (pCurrent) {
    TileOcclusionRendererProxy* pNext = pCurrent->_pNext;
    this->destroyProxy(pCurrent);
    pCurrent = pNext;
  }
}

const TileOcclusionRendererProxy*
TileOcclusionRendererProxyPool::fetchOcclusionProxyForTile(
    const Tile& tile,
    int32_t currentFrame) {
  auto mappingIt = this->_tileToOcclusionProxyMapping.find(&tile);
  if (mappingIt != this->_tileToOcclusionProxyMapping.end()) {
    TileOcclusionRendererProxy* pProxy = mappingIt->second;

    pProxy->update(currentFrame);
    pProxy->_usedLastFrame = true;

    return pProxy;
  }

  TileOcclusionRendererProxy* pAssignedProxy = this->_pFreeProxiesHead;
  this->_pFreeProxiesHead =
      this->_pFreeProxiesHead ? this->_pFreeProxiesHead->_pNext : nullptr;
  pAssignedProxy->_pNext = nullptr;
  pAssignedProxy->_usedLastFrame = true;

  this->_tileToOcclusionProxyMapping.emplace(&tile, pAssignedProxy);

  return pAssignedProxy;
}

void TileOcclusionRendererProxyPool::pruneOcclusionProxyMappings() {
  for (auto& pair : this->_tileToOcclusionProxyMapping) {
    if (!pair.second->_usedLastFrame) {
      // This tile was not traversed last frame, unmap the proxy and re-add it
      // to the free list.
      pair.second->_pNext = this->_pFreeProxiesHead;
      this->_pFreeProxiesHead = pair.second;
      this->_tileToOcclusionProxyMapping.erase(pair.first);
    } else {
      // Reset the flag.
      pair.second->_usedLastFrame = false;
    }
  }
}

} // namespace Cesium3DTilesSelection