
#include "Cesium3DTilesSelection/ImplicitTraversal.h"

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/TileContext.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeometry/QuadtreeRectangleAvailability.h"
#include "QuantizedMeshContent.h"

#include <CesiumGeometry/TileAvailabilityFlags.h>
#include <CesiumGeospatial/S2CellID.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {

ImplicitTraversalInfo::ImplicitTraversalInfo() noexcept
    : pParentNode(nullptr),
      pCurrentNode(nullptr),
      availability(0U),
      usingImplicitQuadtreeTiling(false),
      usingImplicitOctreeTiling(false),
      shouldQueueSubtreeLoad(false) {}

ImplicitTraversalInfo::ImplicitTraversalInfo(
    Tile* pTile,
    const ImplicitTraversalInfo* pParentInfo) noexcept
    : pParentNode(nullptr),
      pCurrentNode(nullptr),
      availability(0U),
      usingImplicitQuadtreeTiling(false),
      usingImplicitOctreeTiling(false),
      shouldQueueSubtreeLoad(false) {

  if (!pTile) {
    return;
  }

  TileContext* pContext = pTile->getContext();
  if (!pContext || !pContext->implicitContext) {
    return;
  }

  const TileID& id = pTile->getTileID();
  const QuadtreeTileID* pQuadtreeID = std::get_if<QuadtreeTileID>(&id);
  const OctreeTileID* pOctreeID = std::get_if<OctreeTileID>(&id);
  const UpsampledQuadtreeNode* pUpsampledNode =
      std::get_if<UpsampledQuadtreeNode>(&id);

  if (pUpsampledNode) {
    // This tile is upsampling from an implicit tileset leaf. It needs no
    // implicit information for itself.
    return;
  }

  if (!pQuadtreeID && !pOctreeID) {
    return;
  }

  ImplicitTilingContext& implicitContext = *pContext->implicitContext;

  // This tile was created and it uses implicit tiling, so the tile is implied
  // to be available.
  // We explicitly consider all tiles here to be at least TILE_AVAILABLE by the
  // fact that they are being traversed. This is built on the assumption that
  // createImplicitChildrenIfNeeded(...) wouldn't have created this tile if it
  // wasn't available.
  this->availability = TileAvailabilityFlags::TILE_AVAILABLE;

  if (!pParentInfo ||
      (!pParentInfo->pParentNode && !pParentInfo->pCurrentNode)) {
    // The parent was not an implicit tile. Check to see if the current tile is
    // the root to an implicit tileset.

    if (pQuadtreeID && pQuadtreeID->level == 0 &&
        implicitContext.quadtreeAvailability) {
      this->pCurrentNode = implicitContext.quadtreeAvailability->getRootNode();
      this->usingImplicitQuadtreeTiling = true;

      if (!this->pCurrentNode) {
        // The root subtree node is unloaded.
        this->shouldQueueSubtreeLoad = true;
      }
    } else if (
        pOctreeID && pOctreeID->level == 0 &&
        implicitContext.octreeAvailability) {
      this->pCurrentNode = implicitContext.octreeAvailability->getRootNode();
      this->usingImplicitOctreeTiling = true;

      if (!this->pCurrentNode) {
        // The root subtree node is unloaded.
        this->shouldQueueSubtreeLoad = true;
      }
    }
  } else if (pParentInfo->pCurrentNode) {
    // The parent implicit info has enough information for us to derive this
    // tile's implicit info.

    if (pParentInfo->usingImplicitQuadtreeTiling) {
      this->usingImplicitQuadtreeTiling = true;

      // The child's availability is in the same subtree node unless it is
      // the root of a child subtree.
      if ((pQuadtreeID->level %
           implicitContext.quadtreeAvailability->getSubtreeLevels()) != 0) {
        this->pParentNode = pParentInfo->pParentNode;
        this->pCurrentNode = pParentInfo->pCurrentNode;
      } else {
        this->pParentNode = pParentInfo->pCurrentNode;
        this->pCurrentNode =
            implicitContext.quadtreeAvailability->findChildNode(
                *pQuadtreeID,
                pParentInfo->pCurrentNode);

        if (!this->pCurrentNode) {
          // This is an unloaded subtree's root tile.
          this->shouldQueueSubtreeLoad = true;
        }
      }
    } else if (pParentInfo->usingImplicitOctreeTiling) {
      this->usingImplicitOctreeTiling = true;

      // The child's availability is in the same subtree node unless it is
      // the root of a child subtree.
      if ((pOctreeID->level %
           implicitContext.octreeAvailability->getSubtreeLevels()) != 0) {
        this->pParentNode = pParentInfo->pParentNode;
        this->pCurrentNode = pParentInfo->pCurrentNode;
      } else {
        this->pParentNode = pParentInfo->pCurrentNode;
        this->pCurrentNode = implicitContext.octreeAvailability->findChildNode(
            *pOctreeID,
            pParentInfo->pCurrentNode);

        if (!this->pCurrentNode) {
          // This is an unloaded subtree's root tile.
          this->shouldQueueSubtreeLoad = true;
        }
      }
    }
  }

  // Compute the availability.
  if (this->usingImplicitQuadtreeTiling) {
    this->availability = static_cast<uint8_t>(
        this->availability |
        implicitContext.quadtreeAvailability->computeAvailability(
            *pQuadtreeID,
            this->pCurrentNode));

  } else if (this->usingImplicitOctreeTiling) {
    this->availability = static_cast<uint8_t>(
        this->availability |
        implicitContext.octreeAvailability->computeAvailability(
            *pOctreeID,
            this->pCurrentNode));
  }
}

namespace {
inline CesiumGeometry::QuadtreeTileID getAvailabilityTile(
    const CesiumGeometry::QuadtreeTileID& tileID,
    uint32_t availabilityLevels) {
  uint32_t parentLevel =
      tileID.level % availabilityLevels == 0
          ? tileID.level - availabilityLevels
          : (tileID.level / availabilityLevels) * availabilityLevels;

  uint32_t divisor = 1U << (tileID.level - parentLevel);
  return QuadtreeTileID(parentLevel, tileID.x / divisor, tileID.y / divisor);
}

std::optional<CesiumGeometry::QuadtreeTileID> getUnloadedAvailabilityTile(
    const TileContext* pContext,
    CesiumGeometry::QuadtreeTileID tileID,
    uint32_t availabilityLevels) {
  while (tileID.level > 0) {
    tileID = getAvailabilityTile(tileID, availabilityLevels);
    if (pContext->availabilityTilesLoaded.find(tileID) ==
            pContext->availabilityTilesLoaded.end() &&
        pContext->implicitContext->rectangleAvailability->isTileAvailable(
            tileID)) {
      return std::make_optional<QuadtreeTileID>(tileID);
    }
  }
  return std::nullopt;
}

void createQuantizedMeshChildren(
    Tile& parentTile,
    TileContext* parentContext,
    const CesiumGeometry::QuadtreeTileID* parentID);

void requestAvailabilityTile(
    Tile& parentTile,
    TileContext* parentContext,
    const CesiumGeometry::QuadtreeTileID* parentID,
    TileContext* childContext,
    const CesiumGeometry::QuadtreeTileID& availabilityTileID) {
  const Cesium3DTilesSelection::TilesetExternals& externals =
      parentTile.getTileset()->getExternals();
  std::string url = childContext->pTileset->getResolvedContentUrl(
      *childContext,
      availabilityTileID);

  ++parentContext->availabilityLoadsInProgress;
  // Make sure that the tileset is not destroyed while availability is loading.
  parentTile.getTileset()->notifyTileStartLoading(nullptr);
  externals.pAssetAccessor
      ->get(externals.asyncSystem, url, childContext->requestHeaders)
      .thenInWorkerThread(
          [pLogger = externals.pLogger, url, availabilityTileID](
              std::shared_ptr<IAssetRequest>&& pRequest)
              -> std::vector<QuadtreeTileRectangularRange> {
            const IAssetResponse* pResponse = pRequest->response();
            if (pResponse) {
              uint16_t statusCode = pResponse->statusCode();

              if (!(statusCode != 0 &&
                    (statusCode < 200 || statusCode >= 300))) {
                return QuantizedMeshContent::loadMetadata(
                    pLogger,
                    pResponse->data(),
                    availabilityTileID);
              }
            }
            return {};
          })
      .thenInMainThread(
          [&parentTile,
           parentContext,
           parentID,
           childContext,
           availabilityTileID](
              std::vector<CesiumGeometry::QuadtreeTileRectangularRange>&&
                  rectangles) {
            --parentTile.getContext()->availabilityLoadsInProgress;
            parentTile.getTileset()->notifyTileDoneLoading(nullptr);
            childContext->availabilityTilesLoaded.insert(availabilityTileID);
            if (!rectangles.empty()) {
              for (const QuadtreeTileRectangularRange& range : rectangles) {
                childContext->implicitContext->rectangleAvailability
                    ->addAvailableTileRange(range);
              }
            }
            createQuantizedMeshChildren(parentTile, parentContext, parentID);
          });
}
void createQuantizedMeshChildren(
    Tile& parentTile,
    TileContext* parentContext,
    const CesiumGeometry::QuadtreeTileID* parentID) {
  const CesiumGeometry::QuadtreeTileID swID(
      parentID->level + 1,
      parentID->x * 2,
      parentID->y * 2);
  const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  const QuadtreeTileID* childIDs[4] = {&swID, &seID, &nwID, &neID};
  uint8_t availabilities[4] = {};

  TileContext* childContexts[4] =
      {parentContext, parentContext, parentContext, parentContext};
  bool anyAvailable = false;

  for (int i = 0; i < 4; i++) {
    const QuadtreeTileID* id = childIDs[i];
    uint8_t& available = availabilities[i];
    TileContext* childContext = childContexts[i];
    while (true) {
      available =
          childContext->implicitContext->rectangleAvailability->isTileAvailable(
              *id);
      if (available) {
        anyAvailable = true;
        childContexts[i] = childContext;
        break;
      }
      if (childContext->availabilityLevels) {
        std::optional<QuadtreeTileID> unloadedAvailabilityTile =
            getUnloadedAvailabilityTile(
                childContext,
                *id,
                *childContext->availabilityLevels);
        if (unloadedAvailabilityTile) {
          // load parent availability tile before going any further
          return requestAvailabilityTile(
              parentTile,
              parentContext,
              parentID,
              childContext,
              *unloadedAvailabilityTile);
        }
      }
      if (childContext->pUnderlyingContext) {
        childContext = childContext->pUnderlyingContext.get();
      } else {
        break;
      }
    }
  }

  if (anyAvailable) {
    // For quantized mesh, if any children are available, we need to create
    // all four in order to avoid holes. But non-available tiles will be
    // upsampled instead of loaded.

    parentTile.createChildTiles(4);
    gsl::span<Tile> children = parentTile.getChildren();

    for (uint32_t i = 0; i < 4; i++) {
      ImplicitTraversalUtilities::createImplicitQuadtreeTile(
          childContexts[i],
          *childContexts[i]->implicitContext,
          parentTile,
          children[i],
          *childIDs[i],
          availabilities[i]);
    }
  }
}
} // namespace

namespace ImplicitTraversalUtilities {

void createImplicitQuadtreeTile(
    const TileContext* tileContext,
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const QuadtreeTileID& childID,
    uint8_t availability) {

  child.setContext(const_cast<TileContext*>(tileContext));
  child.setParent(&parent);
  child.setRefine(parent.getRefine());
  child.setTransform(parent.getTransform());

  if (availability & TileAvailabilityFlags::TILE_AVAILABLE) {
    child.setTileID(childID);
  } else {
    child.setTileID(UpsampledQuadtreeNode{childID});
  }

  // TODO: check override geometric error from metadata
  child.setGeometricError(parent.getGeometricError() * 0.5);

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&implicitContext.implicitRootBoundingVolume);
  const BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<BoundingRegionWithLooseFittingHeights>(
          &implicitContext.implicitRootBoundingVolume);
  const OrientedBoundingBox* pBox = std::get_if<OrientedBoundingBox>(
      &implicitContext.implicitRootBoundingVolume);
  const S2CellBoundingVolume* pS2Cell = std::get_if<S2CellBoundingVolume>(
      &implicitContext.implicitRootBoundingVolume);

  if (!pRegion && pLooseRegion) {
    pRegion = &pLooseRegion->getBoundingRegion();
  }

  if (pRegion && implicitContext.projection &&
      implicitContext.quadtreeTilingScheme) {
    double minimumHeight = -1000.0;
    double maximumHeight = 9000.0;

    const BoundingRegion* pParentRegion =
        std::get_if<BoundingRegion>(&parent.getBoundingVolume());
    if (!pParentRegion) {
      const BoundingRegionWithLooseFittingHeights* pParentLooseRegion =
          std::get_if<BoundingRegionWithLooseFittingHeights>(
              &parent.getBoundingVolume());
      if (pParentLooseRegion) {
        pParentRegion = &pParentLooseRegion->getBoundingRegion();
      }
    }

    if (pParentRegion) {
      minimumHeight = pParentRegion->getMinimumHeight();
      maximumHeight = pParentRegion->getMaximumHeight();
    }

    child.setBoundingVolume(
        BoundingRegionWithLooseFittingHeights(BoundingRegion(
            unprojectRectangleSimple(
                *implicitContext.projection,
                implicitContext.quadtreeTilingScheme->tileToRectangle(childID)),
            minimumHeight,
            maximumHeight)));

  } else if (pBox && implicitContext.quadtreeTilingScheme) {
    CesiumGeometry::Rectangle rectangleLocal =
        implicitContext.quadtreeTilingScheme->tileToRectangle(childID);
    glm::dvec2 centerLocal = rectangleLocal.getCenter();
    const glm::dmat3& rootHalfAxes = pBox->getHalfAxes();
    child.setBoundingVolume(OrientedBoundingBox(
        rootHalfAxes * glm::dvec3(centerLocal.x, centerLocal.y, 0.0),
        glm::dmat3(
            0.5 * rectangleLocal.computeWidth() * rootHalfAxes[0],
            0.5 * rectangleLocal.computeHeight() * rootHalfAxes[1],
            rootHalfAxes[2])));
  } else if (pS2Cell) {
    child.setBoundingVolume(S2CellBoundingVolume(
        S2CellID::fromQuadtreeTileID(pS2Cell->getCellID().getFace(), childID),
        pS2Cell->getMinimumHeight(),
        pS2Cell->getMaximumHeight()));
  }
}

void createImplicitOctreeTile(
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const OctreeTileID& childID,
    uint8_t availability) {

  child.setContext(parent.getContext());
  child.setParent(&parent);
  child.setRefine(parent.getRefine());
  child.setTransform(parent.getTransform());

  if (availability & TileAvailabilityFlags::TILE_AVAILABLE) {
    child.setTileID(childID);
  }

  // TODO: check for overrided geometric error metadata
  child.setGeometricError(parent.getGeometricError() * 0.5);

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&implicitContext.implicitRootBoundingVolume);
  const OrientedBoundingBox* pBox = std::get_if<OrientedBoundingBox>(
      &implicitContext.implicitRootBoundingVolume);
  const S2CellBoundingVolume* pS2Cell = std::get_if<S2CellBoundingVolume>(
      &implicitContext.implicitRootBoundingVolume);

  if (pRegion && implicitContext.projection &&
      implicitContext.octreeTilingScheme) {
    child.setBoundingVolume(unprojectRegionSimple(
        *implicitContext.projection,
        implicitContext.octreeTilingScheme->tileToBox(childID)));
  } else if (pBox && implicitContext.octreeTilingScheme) {
    AxisAlignedBox childLocal =
        implicitContext.octreeTilingScheme->tileToBox(childID);
    const glm::dvec3& centerLocal = childLocal.center;
    const glm::dmat3& rootHalfAxes = pBox->getHalfAxes();
    child.setBoundingVolume(OrientedBoundingBox(
        rootHalfAxes * centerLocal,
        glm::dmat3(
            0.5 * childLocal.lengthX * rootHalfAxes[0],
            0.5 * childLocal.lengthY * rootHalfAxes[1],
            0.5 * childLocal.lengthZ * rootHalfAxes[2])));
  } else if (pS2Cell) {
    // Derive the height directly from the root S2 bounding volume.
    uint32_t tilesAtLevel = static_cast<uint32_t>(1U << childID.level);
    double rootMinHeight = pS2Cell->getMinimumHeight();
    double rootMaxHeight = pS2Cell->getMaximumHeight();
    double tileSizeZ = (rootMaxHeight - rootMinHeight) / tilesAtLevel;

    double tileMinHeight = childID.z * tileSizeZ;

    child.setBoundingVolume(S2CellBoundingVolume(
        S2CellID::fromQuadtreeTileID(
            pS2Cell->getCellID().getFace(),
            QuadtreeTileID(childID.level, childID.x, childID.y)),
        tileMinHeight,
        tileMinHeight + tileSizeZ));
  }
}

void createImplicitChildrenIfNeeded(
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo) {

  TileContext* pContext = tile.getContext();
  if (pContext && pContext->implicitContext && tile.getChildren().empty()) {
    const ImplicitTilingContext& implicitContext =
        pContext->implicitContext.value();
    const QuadtreeTileID* pQuadtreeTileID =
        std::get_if<QuadtreeTileID>(&tile.getTileID());
    const OctreeTileID* pOctreeTileID =
        std::get_if<OctreeTileID>(&tile.getTileID());

    if (pQuadtreeTileID) {
      // Check if any child tiles are known to be available, and create them
      // if they are.

      if (implicitContext.rectangleAvailability) {
        return createQuantizedMeshChildren(tile, pContext, pQuadtreeTileID);
      }
      const QuadtreeTileID swID(
          pQuadtreeTileID->level + 1,
          pQuadtreeTileID->x * 2,
          pQuadtreeTileID->y * 2);
      const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
      const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
      const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

      uint8_t sw = 0;
      uint8_t se = 0;
      uint8_t nw = 0;
      uint8_t ne = 0;

      if (implicitContext.quadtreeAvailability) {
        if ((swID.level %
             implicitContext.quadtreeAvailability->getSubtreeLevels()) == 0) {
          // If the tiles are in child subtrees, we know enough about them to
          // decide whether to create the tile itself and whether each will
          // act as the root to a child subtree, but we do not yet know if
          // they will have content.
          if (implicitContext.quadtreeAvailability->findChildNodeIndex(
                  swID,
                  implicitInfo.pCurrentNode)) {
            sw = TileAvailabilityFlags::TILE_AVAILABLE |
                 TileAvailabilityFlags::SUBTREE_AVAILABLE;
          }

          if (implicitContext.quadtreeAvailability->findChildNodeIndex(
                  seID,
                  implicitInfo.pCurrentNode)) {
            se = TileAvailabilityFlags::TILE_AVAILABLE |
                 TileAvailabilityFlags::SUBTREE_AVAILABLE;
          }

          if (implicitContext.quadtreeAvailability->findChildNodeIndex(
                  nwID,
                  implicitInfo.pCurrentNode)) {
            nw = TileAvailabilityFlags::TILE_AVAILABLE |
                 TileAvailabilityFlags::SUBTREE_AVAILABLE;
          }

          if (implicitContext.quadtreeAvailability->findChildNodeIndex(
                  neID,
                  implicitInfo.pCurrentNode)) {
            ne = TileAvailabilityFlags::TILE_AVAILABLE |
                 TileAvailabilityFlags::SUBTREE_AVAILABLE;
          }
        } else {
          sw = implicitContext.quadtreeAvailability->computeAvailability(
              swID,
              implicitInfo.pCurrentNode);
          se = implicitContext.quadtreeAvailability->computeAvailability(
              seID,
              implicitInfo.pCurrentNode);
          nw = implicitContext.quadtreeAvailability->computeAvailability(
              nwID,
              implicitInfo.pCurrentNode);
          ne = implicitContext.quadtreeAvailability->computeAvailability(
              neID,
              implicitInfo.pCurrentNode);
        }
      }

      size_t childCount = static_cast<size_t>(
          (sw & TileAvailabilityFlags::TILE_AVAILABLE) +
          (se & TileAvailabilityFlags::TILE_AVAILABLE) +
          (nw & TileAvailabilityFlags::TILE_AVAILABLE) +
          (ne & TileAvailabilityFlags::TILE_AVAILABLE));

      if (implicitContext.quadtreeAvailability) {

        tile.createChildTiles(childCount);
        gsl::span<Tile> children = tile.getChildren();
        size_t childIndex = 0;

        if (sw & TileAvailabilityFlags::TILE_AVAILABLE) {
          createImplicitQuadtreeTile(
              pContext,
              implicitContext,
              tile,
              children[childIndex++],
              swID,
              sw);
        }

        if (se & TileAvailabilityFlags::TILE_AVAILABLE) {
          createImplicitQuadtreeTile(
              pContext,
              implicitContext,
              tile,
              children[childIndex++],
              seID,
              se);
        }

        if (nw & TileAvailabilityFlags::TILE_AVAILABLE) {
          createImplicitQuadtreeTile(
              pContext,
              implicitContext,
              tile,
              children[childIndex++],
              nwID,
              nw);
        }

        if (ne & TileAvailabilityFlags::TILE_AVAILABLE) {
          createImplicitQuadtreeTile(
              pContext,
              implicitContext,
              tile,
              children[childIndex],
              neID,
              ne);
        }
      }

    } else if (pOctreeTileID && implicitContext.octreeAvailability) {
      // Check if any child tiles are known to be available, and create them
      // if they are.

      uint8_t availabilities[8];
      OctreeTileID childIDs[8];

      OctreeTileID firstChildID(
          pOctreeTileID->level + 1,
          pOctreeTileID->x * 2,
          pOctreeTileID->y * 2,
          pOctreeTileID->z * 2);

      uint8_t availableChildren = 0;

      if ((childIDs[0].level %
           implicitContext.octreeAvailability->getSubtreeLevels()) == 0) {
        for (uint8_t relativeChildID = 0; relativeChildID < 8;
             ++relativeChildID) {
          childIDs[relativeChildID] = OctreeTileID(
              firstChildID.level,
              firstChildID.x + ((relativeChildID & 4) >> 2),
              firstChildID.y + ((relativeChildID & 2) >> 1),
              firstChildID.z + (relativeChildID & 1));

          if (implicitContext.octreeAvailability->findChildNodeIndex(
                  childIDs[relativeChildID],
                  implicitInfo.pCurrentNode)) {
            availabilities[relativeChildID] =
                TileAvailabilityFlags::TILE_AVAILABLE |
                TileAvailabilityFlags::SUBTREE_AVAILABLE;
          }

          if (availabilities[relativeChildID] &
              TileAvailabilityFlags::TILE_AVAILABLE) {
            ++availableChildren;
          }
        }
      } else {
        for (uint8_t relativeChildID = 0; relativeChildID < 8;
             ++relativeChildID) {
          childIDs[relativeChildID] = OctreeTileID(
              firstChildID.level,
              firstChildID.x + ((relativeChildID & 4) >> 2),
              firstChildID.y + ((relativeChildID & 2) >> 1),
              firstChildID.z + (relativeChildID & 1));

          availabilities[relativeChildID] =
              implicitContext.octreeAvailability->computeAvailability(
                  childIDs[relativeChildID],
                  implicitInfo.pCurrentNode);

          if (availabilities[relativeChildID] &
              TileAvailabilityFlags::TILE_AVAILABLE) {
            ++availableChildren;
          }
        }
      }

      tile.createChildTiles(availableChildren);
      gsl::span<Tile> children = tile.getChildren();

      for (uint8_t relativeChildId = 0, availableChild = 0; relativeChildId < 8;
           ++relativeChildId) {
        uint8_t availability = availabilities[relativeChildId];
        if (availability & TileAvailabilityFlags::TILE_AVAILABLE) {
          createImplicitOctreeTile(
              implicitContext,
              tile,
              children[availableChild++],
              childIDs[relativeChildId],
              availabilities[relativeChildId]);
        }
      }
    }
  }
}
} // namespace ImplicitTraversalUtilities
} // namespace Cesium3DTilesSelection
