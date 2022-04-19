#pragma once

#include "Library.h"
#include "Tile.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Cesium3DTilesSelection {

class CESIUM3DTILESSELECTION_API TileOcclusionRendererProxy {
public:
  /**
   * @brief Whether the tile's bounding volume is currently occluded.
   *
   * @return The occlusion state.
   */
  virtual bool isOccluded() const;

  /**
   * @brief Get the last frame where a definitive occlusion state was
   * determined.
   *
   * @return The frame number.
   */
  virtual int32_t getLastUpdatedFrame() const;

protected:
  friend class TileOcclusionRendererProxyPool;

  /**
   * @brief Reset this proxy to target a new tile. If nullptr, this proxy is
   * back in the pool and will not be used for further occlusion until reset
   * is called again with an actual tile.
   *
   * @param pTile The tile that this proxy represents or nullptr if the proxy
   * is back in the pool.
   */
  virtual void reset(const Tile* pTile);

  /**
   * @brief Update the tile occlusion proxy.
   *
   * @param currentFrame The current frame number.
   */
  virtual void update(int32_t currentFrame);

private:
  friend class TileOcclusionRendererProxyPool;

  bool _usedLastFrame = false;
  TileOcclusionRendererProxy* _pNext;
};

class CESIUM3DTILESSELECTION_API TileOcclusionRendererProxyPool {
public:
  /**
   * @brief Initialize a pool of {@link TileOcclusionRendererProxy}s of the
   * given size.
   *
   * @param poolSize The pool size.
   */
  void initPool(int32_t poolSize);

  /**
   * @brief Destroy the pool.
   */
  void destroyPool();

  /**
   * @brief Get the {@link TileOcclusionRendererProxy} mapped to the tile.
   * Attempts to create a new mapping if one does not exist already by
   * assigning a proxy from the free list.
   *
   * @param tile The tile.
   * @param currentFrame The current frame number.
   * @return The occlusion proxy mapped to this tile, or nullptr if one can't
   * be made.
   */
  const TileOcclusionRendererProxy*
  fetchOcclusionProxyForTile(const Tile& tile, int32_t currentFrame);

  /**
   * @brief Prunes the occlusion proxy mappings and removes any mappings that
   * were unused the last frame. Any mapping corresponding to a tile that was
   * not visited will have been unused. Occlusion proxies from removed mappings
   * will be returned to the free list.
   */
  void pruneOcclusionProxyMappings();

protected:
  /**
   * @brief Create a {@link TileOcclusionRendererProxy}.
   *
   * @return A new occlusion proxy.
   */
  virtual TileOcclusionRendererProxy* createProxy();

  /**
   * @brief Destroy a {@link TileOcclusionRendererProxy} that is done being used.
   *
   * @param pProxy The proxy to be destroyed.
   */
  virtual void destroyProxy(TileOcclusionRendererProxy* pProxy);

private:
  // Singly linked list representing the free proxies in the pool
  TileOcclusionRendererProxy* _pFreeProxiesHead;
  // The currently used proxies in the pool
  std::unordered_map<const Tile*, TileOcclusionRendererProxy*>
      _tileToOcclusionProxyMapping;
};

} // namespace Cesium3DTilesSelection
