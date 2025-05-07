#pragma once

#include "CesiumUtility/IntrusivePointer.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/ReferenceCounted.h>

namespace Cesium3DTilesSelection {
class TileRefinementMonitor {
public:
  virtual bool isTileRelevant(const Cesium3DTilesSelection::Tile&) = 0;
  virtual void
  onTileRefinementChanged(const Cesium3DTilesSelection::Tile&) = 0;

  virtual ~TileRefinementMonitor() {
    for (Tile* pTile : this->_attachedTiles) {
      auto it = std::find_if(
          pTile->_attachedRefinementMonitors.begin(),
          pTile->_attachedRefinementMonitors.end(),
          [this](std::shared_ptr<TileRefinementMonitor>&
                     pOtherMonitor) { return pOtherMonitor.get() == this; });
      CESIUM_ASSERT(it != pTile->_attachedRefinementMonitors.end());
      pTile->_attachedRefinementMonitors.erase(it);
    }
  }

private:
  void onTileDestroy(const Tile* pTile) {
    this->_attachedTiles.erase(
        std::remove_if(
            this->_attachedTiles.begin(),
            this->_attachedTiles.end(),
            [pTile](const Tile* pTileCandidate) {
              return pTile == pTileCandidate;
            }),
        this->_attachedTiles.end());
  }

  std::vector<Tile*> _attachedTiles;

  friend class Tile;
  friend class Tileset;
};
} // namespace Cesium3DTilesSelection