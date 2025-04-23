#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeospatial/BoundingRegionBuilder.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/CompositeCartographicPolygon.h"
#include "CesiumVectorData/VectorNode.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/VectorDocumentRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>
#include <spdlog/fwd.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumRasterOverlays {

namespace {
double computeLevelDenominator(uint32_t level) noexcept {
  return static_cast<double>(1 << level);
}

struct VectorQuadtreeNode {
  VectorQuadtreeNode(GlobeRectangle rootGlobeRectangle, QuadtreeTileID id)
      : nodeId(id),
        primitives(),
        children(),
        rectangle(rootGlobeRectangle.subdivideRectangle(
            id,
            computeLevelDenominator(id.level))) {}

  QuadtreeTileID nodeId;
  std::vector<const VectorPrimitive*> primitives;
  std::vector<std::unique_ptr<VectorQuadtreeNode>> children;
  CesiumGeospatial::GlobeRectangle rectangle;
};

struct GlobeRectangleFromPrimitiveVisitor {
  GlobeRectangle
  operator()(const CesiumGeospatial::Cartographic& cartographic) {
    return GlobeRectangle(
        cartographic.longitude,
        cartographic.latitude,
        cartographic.longitude,
        cartographic.latitude);
  }

  GlobeRectangle
  operator()(const std::vector<CesiumGeospatial::Cartographic>& points) {
    BoundingRegionBuilder builder;
    for (const Cartographic& point : points) {
      builder.expandToIncludePosition(point);
    }
    return builder.toGlobeRectangle();
  }

  GlobeRectangle
  operator()(const CesiumGeospatial::CompositeCartographicPolygon& polygon) {
    return polygon.getBoundingRectangle();
  }
};

// TODO: not a very optimized algorithm for generating a quadtree
void addPrimitiveToQuadtree(
    const VectorPrimitive& primitive,
    const GlobeRectangle& primitiveRectangle,
    const GlobeRectangle& rootGlobeRectangle,
    VectorQuadtreeNode& node) {
  if (!node.rectangle.computeIntersection(primitiveRectangle)) {
    // They don't touch, nothing to do.
    return;
  }

  const QuadtreeTileID& nodeId = node.nodeId;

  node.primitives.emplace_back(&primitive);

  if (node.children.empty() && node.primitives.size() < 2) {
    // Not enough primitives here to make a new child.
    return;
  } else if (node.children.empty() && node.primitives.size() >= 2) {
    // Enough room to create children.
    node.children.emplace_back(std::make_unique<VectorQuadtreeNode>(
        rootGlobeRectangle,
        QuadtreeTileID{nodeId.level + 1, nodeId.x * 2, nodeId.y * 2}));
    node.children.emplace_back(std::make_unique<VectorQuadtreeNode>(
        rootGlobeRectangle,
        QuadtreeTileID{nodeId.level + 1, nodeId.x * 2 + 1, nodeId.y * 2}));
    node.children.emplace_back(std::make_unique<VectorQuadtreeNode>(
        rootGlobeRectangle,
        QuadtreeTileID{nodeId.level + 1, nodeId.x * 2, nodeId.y * 2 + 1}));
    node.children.emplace_back(std::make_unique<VectorQuadtreeNode>(
        rootGlobeRectangle,
        QuadtreeTileID{nodeId.level + 1, nodeId.x * 2 + 1, nodeId.y * 2 + 1}));
    for (const VectorPrimitive* pPrimitive : node.primitives) {
      addPrimitiveToQuadtree(
          *pPrimitive,
          std::visit(GlobeRectangleFromPrimitiveVisitor{}, *pPrimitive),
          rootGlobeRectangle,
          *node.children[0]);
      addPrimitiveToQuadtree(
          *pPrimitive,
          std::visit(GlobeRectangleFromPrimitiveVisitor{}, *pPrimitive),
          rootGlobeRectangle,
          *node.children[1]);
      addPrimitiveToQuadtree(
          *pPrimitive,
          std::visit(GlobeRectangleFromPrimitiveVisitor{}, *pPrimitive),
          rootGlobeRectangle,
          *node.children[2]);
      addPrimitiveToQuadtree(
          *pPrimitive,
          std::visit(GlobeRectangleFromPrimitiveVisitor{}, *pPrimitive),
          rootGlobeRectangle,
          *node.children[3]);
    }
  } else {
    addPrimitiveToQuadtree(
        primitive,
        primitiveRectangle,
        rootGlobeRectangle,
        *node.children[0]);
    addPrimitiveToQuadtree(
        primitive,
        primitiveRectangle,
        rootGlobeRectangle,
        *node.children[1]);
    addPrimitiveToQuadtree(
        primitive,
        primitiveRectangle,
        rootGlobeRectangle,
        *node.children[2]);
    addPrimitiveToQuadtree(
        primitive,
        primitiveRectangle,
        rootGlobeRectangle,
        *node.children[3]);
  }
}

void addNodeToQuadtree(
    const VectorNode& vectorNode,
    const GlobeRectangle& rootGlobeRectangle,
    VectorQuadtreeNode& rootQuadtreeNode) {
  for (const VectorPrimitive& primitive : vectorNode.primitives) {
    addPrimitiveToQuadtree(
        primitive,
        std::visit(GlobeRectangleFromPrimitiveVisitor{}, primitive),
        rootGlobeRectangle,
        rootQuadtreeNode);
  }

  for (const VectorNode& child : vectorNode.children) {
    addNodeToQuadtree(child, rootGlobeRectangle, rootQuadtreeNode);
  }
}

void addVectorNodeToRegionBuilder(
    BoundingRegionBuilder& builder,
    const VectorNode& node) {
  for (const VectorPrimitive& primitive : node.primitives) {
    GlobeRectangle rect =
        std::visit(GlobeRectangleFromPrimitiveVisitor{}, primitive);
    builder.expandToIncludePosition(rect.getSouthwest());
    builder.expandToIncludePosition(rect.getNortheast());
  }

  for (const VectorNode& child : node.children) {
    addVectorNodeToRegionBuilder(builder, child);
  }
}

GlobeRectangle
globeRectangleForDocument(const IntrusivePointer<VectorDocument>& document) {
  BoundingRegionBuilder builder;
  addVectorNodeToRegionBuilder(builder, document->getRootNode());
  return builder.toGlobeRectangle();
}

VectorQuadtreeNode buildQuadtree(
    const IntrusivePointer<VectorDocument>& document,
    const GlobeRectangle& globeRectangle) {
  VectorQuadtreeNode root{globeRectangle, QuadtreeTileID{0, 0, 0}};
  addNodeToQuadtree(document->getRootNode(), globeRectangle, root);
  return root;
}

void rasterizeQuadtreeNode(
    const VectorQuadtreeNode& quadtreeNode,
    const GlobeRectangle& rectangle,
    const Color& color,
    VectorRasterizer& rasterizer) {
  // If this node has no children, or if it is entirely within the target
  // rectangle, let's rasterize this node's contents and not any children.
  if (quadtreeNode.children.empty() ||
      (rectangle.contains(quadtreeNode.rectangle.getSouthwest()) &&
       rectangle.contains(quadtreeNode.rectangle.getNortheast()))) {
    for (const VectorPrimitive* pPrimitive : quadtreeNode.primitives) {
      rasterizer.drawPrimitive(*pPrimitive, color);
    }
  } else {
    for (size_t i = 0; i < quadtreeNode.children.size(); i++) {
      if (rectangle.computeIntersection(quadtreeNode.children[i]->rectangle)) {
        rasterizeQuadtreeNode(
            *quadtreeNode.children[i],
            rectangle,
            color,
            rasterizer);
      }
    }
  }
}

void rasterizeVectorData(
    LoadedRasterOverlayImage& result,
    const GlobeRectangle& rectangle,
    const VectorQuadtreeNode& rootQuadtreeNode,
    const Color& color) {
  VectorRasterizer rasterizer(rectangle, result.pImage);
  rasterizeQuadtreeNode(rootQuadtreeNode, rectangle, color, rasterizer);
  result.pImage = rasterizer.finalize();
}
} // namespace

class CESIUMRASTEROVERLAYS_API VectorDocumentRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {

private:
  IntrusivePointer<VectorDocument> _document;
  GlobeRectangle _contentsRectangle;
  VectorQuadtreeNode _rootQuadtreeNode;
  Color _color;

public:
  VectorDocumentRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>&
          document,
      const CesiumVectorData::Color& color)
      : RasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            projection,
            // computeCoverageRectangle(projection, polygons)),
            projectRectangleSimple(
                projection,
                CesiumGeospatial::GlobeRectangle::MAXIMUM)),
        _document(document),
        _contentsRectangle(globeRectangleForDocument(this->_document)),
        _rootQuadtreeNode(
            buildQuadtree(this->_document, this->_contentsRectangle)),
        _color(color) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) override {
    // Choose the texture size according to the geometry screen size and raster
    // SSE, but no larger than the maximum texture size.
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::ivec2 textureSize = glm::min(
        glm::ivec2(
            overlayTile.getTargetScreenPixels() /
            options.maximumScreenSpaceError),
        glm::ivec2(options.maximumTextureSize));

    return this->getAsyncSystem().runInWorkerThread(
        [document = this->_document,
         &rootQuadtreeNode = this->_rootQuadtreeNode,
         contentsRectangle = this->_contentsRectangle,
         _color = this->_color,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle(),
         textureSize]() -> LoadedRasterOverlayImage {
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage result;
          result.rectangle = rectangle;

          if (!tileRectangle.computeIntersection(contentsRectangle)) {
            // Transparent square if this is outside of the contents of this
            // vector document.
            result.moreDetailAvailable = false;
            result.pImage.emplace();
            result.pImage->width = 1;
            result.pImage->height = 1;
            result.pImage->channels = 4;
            result.pImage->bytesPerChannel = 1;
            result.pImage->pixelData = {
                std::byte{0x00},
                std::byte{0x00},
                std::byte{0x00},
                std::byte{0x00}};
          } else {
            result.moreDetailAvailable = true;
            result.pImage.emplace();
            result.pImage->width = textureSize.x;
            result.pImage->height = textureSize.y;
            result.pImage->channels = 4;
            result.pImage->bytesPerChannel = 1;
            rasterizeVectorData(
                result,
                tileRectangle,
                rootQuadtreeNode,
                _color);
          }

          return result;
        });
  }
};

VectorDocumentRasterOverlay::VectorDocumentRasterOverlay(
    const std::string& name,
    const CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>&
        document,
    const CesiumVectorData::Color& color,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::Projection& projection,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _document(document),
      _color(color),
      _ellipsoid(ellipsoid),
      _projection(projection) {}

VectorDocumentRasterOverlay::~VectorDocumentRasterOverlay() = default;

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
VectorDocumentRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& /*pCreditSystem*/,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  pOwner = pOwner ? pOwner : this;

  return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
      IntrusivePointer<RasterOverlayTileProvider>(
          new VectorDocumentRasterOverlayTileProvider(
              pOwner,
              asyncSystem,
              pAssetAccessor,
              pPrepareRendererResources,
              pLogger,
              this->_projection,
              this->_document,
              this->_color)));
}

} // namespace CesiumRasterOverlays
