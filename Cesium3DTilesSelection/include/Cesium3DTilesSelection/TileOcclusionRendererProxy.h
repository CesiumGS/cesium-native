#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/Tile.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief The occlusion state of a tile as reported by the renderer proxy.
 */
enum class CESIUM3DTILESSELECTION_API TileOcclusionState {
  /**
   * @brief The renderer does not yet know if the tile's bounding volume is
   * occluded or not.
   *
   * This can be due to the typical occlusion delay caused by buffered
   * rendering or otherwise be due to postponed occlusion queries. We can
   * choose to wait for the occlusion information to become available before
   * commiting to load the tile. This might prevent unneeded tile loads at the
   * cost of a small delay.
   */
  OcclusionUnavailable,

  /**
   * @brief The tile's bounding volume is known by the renderer to be visible.
   */
  NotOccluded,

  /**
   * @brief The tile's bounding volume is known by the renderer to be occluded.
   */
  Occluded
};

/**
 * @brief An interface for client renderers to use to represent tile bounding
 * volumes that should be occlusion tested.
 */
class CESIUM3DTILESSELECTION_API TileOcclusionRendererProxy {
public:
  /**
   * @brief Get the occlusion state for this tile. If this is
   * OcclusionUnavailable, the traversal may decide to wait for the occlusion
   * result to become available in future frames.
   *
   * Client implementation note: Do not return OcclusionUnavailable if the
   * occlusion for this tile will _never_ become available, otherwise the tile
   * may not refine while waiting for occlusion. In such a case return
   * NotOccluded so the traversal can assume it is _known_ to be visible.
   *
   * @return The occlusion state of this tile.
   */
  virtual TileOcclusionState getOcclusionState() const = 0;

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

/**
 * @brief A pool of {@link TileOcclusionRendererProxy} objects. Allows quick
 * remapping of tiles to occlusion renderer proxies so new proxies do not have
 * to be created for each new tile requesting occlusion results.
 */
class CESIUM3DTILESSELECTION_API TileOcclusionRendererProxyPool {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param maximumPoolSize The maximum number of
   * {@link TileOcclusionRendererProxy} instances that may exist in this pool.
   */
  TileOcclusionRendererProxyPool(int32_t maximumPoolSize);

  /**
   * @brief Destroys this pool.
   */
  virtual ~TileOcclusionRendererProxyPool();

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
  TileOcclusionRendererProxy* _pFreeProxiesHead;
  int32_t _currentSize;
  int32_t _maxSize;
  // The currently used proxies in the pool
  std::unordered_map<const Tile*, TileOcclusionRendererProxy*>
      _tileToOcclusionProxyMappings;
};

} // namespace Cesium3DTilesSelection
