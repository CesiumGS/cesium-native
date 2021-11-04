
#include "ImplicitTraversalInfo.h"

#include "Cesium3DTilesSelection/TileContext.h"
#include "CesiumGeometry/TileAvailabilityFlags.h"

using namespace CesiumGeometry;

namespace Cesium3DTilesSelection {

/**
 * @brief Creates empty instance.
 */
ImplicitTraversalInfo::ImplicitTraversalInfo() noexcept
    : pParentNode(nullptr), pCurrentNode(nullptr), availability(0){};

/**
 * @brief Attempt to initialize an instance with the given {@link Tile}.
 *
 * @param pTile The tile that may be the implicit root.
 */
ImplicitTraversalInfo::ImplicitTraversalInfo(Tile* pTile) noexcept
    : pParentNode(nullptr), pCurrentNode(nullptr), availability(0) {

  if (!pTile) {
    return;
  }

  TileContext* pContext = pTile->getContext();
  if (!pContext || !pContext->implicitContext) {
    return;
  }

  const TileID& id = pTile->getTileID();
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&id);
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&id);

  ImplicitTilingContext& implicitContext = *pContext->implicitContext;
  if (pQuadtreeID && pQuadtreeID->level == 0 &&
      implicitContext.quadtreeAvailability) {
    this->pCurrentNode = implicitContext.quadtreeAvailability->getRootNode();
  } else if (
      pOctreeID && pOctreeID->level == 0 &&
      implicitContext.octreeAvailability) {
    this->pCurrentNode = implicitContext.octreeAvailability->getRootNode();
  }

  if (this->pCurrentNode) {
    this->availability |= CesiumGeometry::TileAvailabilityFlags::TILE_AVAILABLE;
    this->availability |=
        CesiumGeometry::TileAvailabilityFlags::SUBTREE_AVAILABLE;
  }
}
} // namespace Cesium3DTilesSelection