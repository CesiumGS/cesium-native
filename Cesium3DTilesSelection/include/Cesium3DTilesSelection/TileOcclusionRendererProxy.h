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
  virtual bool isOccluded() const = 0;

  /**
   * @brief Whether the tile has valid occlusion info available. If this is
   * false, the traversal may decide to wait for the occlusion result to become
   * available in future frames.
   *
   * Client implementation note: Do not return false if the occlusion for this
   * tile will _never_ become available, otherwise the tile may not refine
   * while waiting for occlusion. In such a case return true here and return
   * false for isOccluded, so the traversal treats the tile as if it is _known_
   * to be unoccluded.
   *
   * @return Whether this tile has valid occlusion info available.
   */
  virtual bool isOcclusionAvailable() const = 0;

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
  virtual void reset(const Tile* pTile) = 0;

private:
  bool _usedLastFrame = false;
  TileOcclusionRendererProxy* _pNext = nullptr;
};

class CESIUM3DTILESSELECTION_API TileOcclusionRendererProxyPool {
public:
  virtual ~TileOcclusionRendererProxyPool(){};

  /**
   * @brief Initialize a pool of {@link TileOcclusionRendererProxy}s of the
   * given size.
   *
   * @param poolSize The pool size.
   */
  void initPool(uint32_t poolSize);

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
  virtual TileOcclusionRendererProxy* createProxy() = 0;

  /**
   * @brief Destroy a {@link TileOcclusionRendererProxy} that is done being used.
   *
   * @param pProxy The proxy to be destroyed.
   */
  virtual void destroyProxy(TileOcclusionRendererProxy* pProxy) = 0;

private:
  // Singly linked list representing the free proxies in the pool
  TileOcclusionRendererProxy* _pFreeProxiesHead = nullptr;
  // The currently used proxies in the pool
  std::unordered_map<const Tile*, TileOcclusionRendererProxy*>
      _tileToOcclusionProxyMappings;
};

} // namespace Cesium3DTilesSelection
