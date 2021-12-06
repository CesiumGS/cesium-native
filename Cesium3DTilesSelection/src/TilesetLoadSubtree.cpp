#include "TilesetLoadSubtree.h"

#include "AvailabilitySubtreeContent.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeometry;

struct Tileset::LoadSubtree::Private {
  static Future<std::shared_ptr<IAssetRequest>>
  requestAvailabilitySubtree(const Tileset& tileset, Tile& tile);

  static std::string getResolvedSubtreeUrl(const Tile& tile);
};

Future<void> Tileset::LoadSubtree::start(
    Tileset& tileset,
    const SubtreeLoadRecord& loadRecord) {
  if (!loadRecord.pTile) {
    return tileset.getAsyncSystem().createResolvedFuture();
  }

  ImplicitTilingContext& implicitContext =
      *loadRecord.pTile->getContext()->implicitContext;

  const TileID& tileID = loadRecord.pTile->getTileID();
  const QuadtreeTileID* pQuadtreeID = std::get_if<QuadtreeTileID>(&tileID);
  const OctreeTileID* pOctreeID = std::get_if<OctreeTileID>(&tileID);

  AvailabilityNode* pNewNode = nullptr;

  if (pQuadtreeID && implicitContext.quadtreeAvailability) {
    pNewNode = implicitContext.quadtreeAvailability->addNode(
        *pQuadtreeID,
        loadRecord.implicitInfo.pParentNode);
  } else if (pOctreeID && implicitContext.octreeAvailability) {
    pNewNode = implicitContext.octreeAvailability->addNode(
        *pOctreeID,
        loadRecord.implicitInfo.pParentNode);
  }

  return Private::requestAvailabilitySubtree(tileset, *loadRecord.pTile)
      .thenInWorkerThread(
          [asyncSystem = tileset.getAsyncSystem(),
           pLogger = tileset.getExternals().pLogger,
           pAssetAccessor = tileset.getExternals().pAssetAccessor](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            const IAssetResponse* pResponse = pRequest->response();

            if (pResponse) {
              uint16_t statusCode = pResponse->statusCode();
              if (statusCode == 0 || (statusCode >= 200 && statusCode < 300)) {
                return AvailabilitySubtreeContent::load(
                    asyncSystem,
                    pLogger,
                    pRequest->url(),
                    pResponse->data(),
                    pAssetAccessor,
                    pRequest->headers());
              }
            }

            return asyncSystem.createResolvedFuture(
                std::unique_ptr<AvailabilitySubtree>(nullptr));
          })
      .thenInMainThread(
          [loadRecord,
           pNewNode](std::unique_ptr<AvailabilitySubtree>&& pSubtree) mutable {
            if (loadRecord.pTile && pNewNode) {
              TileContext* pContext = loadRecord.pTile->getContext();
              if (pContext && pContext->implicitContext) {
                ImplicitTilingContext& implicitContext =
                    *pContext->implicitContext;
                if (loadRecord.implicitInfo.usingImplicitQuadtreeTiling) {
                  implicitContext.quadtreeAvailability->addLoadedSubtree(
                      pNewNode,
                      std::move(*pSubtree.release()));
                } else if (loadRecord.implicitInfo.usingImplicitOctreeTiling) {
                  implicitContext.octreeAvailability->addLoadedSubtree(
                      pNewNode,
                      std::move(*pSubtree.release()));
                }
              }
            }
          })
      .catchInMainThread([&tileset, &tileID](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            tileset.getExternals().pLogger,
            "Unhandled error while loading the subtree for tile id {}: {}",
            TileIdUtilities::createTileIdString(tileID),
            e.what());
      });
}

Future<std::shared_ptr<IAssetRequest>>
Tileset::LoadSubtree::Private::requestAvailabilitySubtree(
    const Tileset& tileset,
    Tile& tile) {
  std::string url = Private::getResolvedSubtreeUrl(tile);
  assert(!url.empty());

  return tileset.getExternals().pAssetAccessor->requestAsset(
      tileset.getAsyncSystem(),
      url,
      tile.getContext()->requestHeaders);
}

std::string
Tileset::LoadSubtree::Private::getResolvedSubtreeUrl(const Tile& tile) {
  struct Operation {
    const TileContext& context;

    std::string operator()(const std::string& url) { return url; }

    std::string operator()(const QuadtreeTileID& quadtreeID) {
      if (!this->context.implicitContext ||
          !this->context.implicitContext->subtreeTemplateUrl) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          *this->context.implicitContext.value().subtreeTemplateUrl,
          [this, &quadtreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level" || placeholder == "z") {
              return std::to_string(quadtreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(quadtreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(quadtreeID.y);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(const OctreeTileID& octreeID) {
      if (!this->context.implicitContext ||
          !this->context.implicitContext->subtreeTemplateUrl) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          *this->context.implicitContext.value().subtreeTemplateUrl,
          [this, &octreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level") {
              return std::to_string(octreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(octreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(octreeID.y);
            }
            if (placeholder == "z") {
              return std::to_string(octreeID.z);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(UpsampledQuadtreeNode /*subdividedParent*/) {
      return std::string();
    }
  };

  std::string url = std::visit(Operation{*tile.getContext()}, tile.getTileID());
  if (url.empty()) {
    return url;
  }

  return CesiumUtility::Uri::resolve(tile.getContext()->baseUrl, url, true);
}
