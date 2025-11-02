#include "CesiumGeometry/Rectangle.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorRasterizer.h>
#include <CesiumVectorData/VectorStyle.h>

#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumRasterOverlays {

namespace {
/**
 * @brief A single geometry object in a GeoJSON file, with all the information
 * required for rendering.
 */
struct QuadtreeGeometryData {
  /**
   * @brief A pointer to the geometry object to render.
   */
  const GeoJsonObject* pObject;
  /**
   * @brief A pointer to the `VectorStyle` to apply to this geometry object.
   */
  const VectorStyle* pStyle;
  /**
   * @brief The bounding rectangle encompassing this geometry.
   */
  GlobeRectangle rectangle;
};

struct QuadtreeNode {
  /**
   * @brief The `GlobeRectangle` defining the bounds of this node.
   */
  GlobeRectangle rectangle;
  /**
   * @brief Indices representing the children of this quadtree node.
   *
   * `0` represents no child, as the 0 index will always be the root node and
   * the root node cannot ever be a child of another node.
   */
  uint32_t children[2][2] = {{0, 0}, {0, 0}};

  QuadtreeNode(const GlobeRectangle& rectangle_) : rectangle(rectangle_) {}

  /**
   * @brief Returns `true` if this node has any children.
   */
  bool anyChildren() const {
    return children[0][0] != 0 && children[0][1] != 0 && children[1][0] != 0 &&
           children[1][1] != 0;
  }
};

/**
 * @brief A quadtree used to speed up the selection of GeoJSON objects to
 * rasterize.
 *
 * A GeoJSON document, unlike something like 3D Tiles or a Tile Map Service, is
 * a format that is not designed around efficient real-time rendering. There is
 * no way to tell from the structure of a GeoJSON document which objects will
 * need to be considered when rendering any particular area. This becomes an
 * issue when working with GeoJSON documents that have thousands, or tens of
 * thousands, or hundreds of thousands of objects that each could be considered
 * for rendering in any particular tile. Performing bounding box comparisons of
 * hundreds of thousands of objects per tile is not conducive to good
 * performance.
 *
 * To speed up these checks, we use a quadtree to speed up our queries against
 * the document. The root level of the quadtree encompasses the entire bounding
 * rectangle of the document. Each sub-level then represents one quarter of the
 * parent level. The same object may appear in multiple levels at once; a line
 * that crosses the north side of one level will appear in both its north east
 * and north west sublevels, and every object that can be rendered will be
 * included in the root level.
 *
 * The reason for including the same object across multiple levels is so that
 * rendering can do a minimum amount of recursing through the tree. If we need
 * to render the entire document at once, for example, the root level will
 * provide the full list of objects to render without having to check any
 * children.
 */
struct Quadtree {
  GlobeRectangle rectangle = GlobeRectangle::EMPTY;
  uint32_t rootId = 0;
  std::vector<QuadtreeNode> nodes;
  std::vector<QuadtreeGeometryData> data;
  /**
   * @brief A vector containing all the geometry of all the nodes.
   *
   * Each element in this vector is an index into `data`.
   */
  std::vector<uint32_t> dataIndices;
  /**
   * @brief A vector containing the first index into `dataIndices` for every
   * node.
   *
   * This vector contains `nodeCount + 1` items, with a synthetic final index
   * added so that `nodeIndex + 1` will always represent the exclusive end of
   * the node's geometry.
   *
   * For example, for the node at index `i`, the indices into `dataIndices`
   * representing its geometry will be the range `[ dataNodeIndicesBegin[i],
   * dataNodeIndicesBegin[i + 1] ]`.
   */
  std::vector<uint32_t> dataNodeIndicesBegin;
};

struct GlobeRectangleFromObjectVisitor {
  BoundingRegionBuilder& builder;
  void operator()(const GeoJsonPoint& point) {
    builder.expandToIncludePosition(
        Cartographic::fromDegrees(point.coordinates.x, point.coordinates.y));
  }
  void operator()(const GeoJsonMultiPoint& points) {
    for (const glm::dvec3& point : points.coordinates) {
      builder.expandToIncludePosition(
          Cartographic::fromDegrees(point.x, point.y));
    }
  }
  void operator()(const GeoJsonLineString& line) {
    for (const glm::dvec3& point : line.coordinates) {
      builder.expandToIncludePosition(
          Cartographic::fromDegrees(point.x, point.y));
    }
  }
  void operator()(const GeoJsonMultiLineString& lines) {
    for (const std::vector<glm::dvec3>& line : lines.coordinates) {
      for (const glm::dvec3& point : line) {
        builder.expandToIncludePosition(
            Cartographic::fromDegrees(point.x, point.y));
      }
    }
  }
  void operator()(const GeoJsonPolygon& polygon) {
    for (const std::vector<glm::dvec3>& ring : polygon.coordinates) {
      for (const glm::dvec3& point : ring) {
        builder.expandToIncludePosition(
            Cartographic::fromDegrees(point.x, point.y));
      }
    }
  }
  void operator()(const GeoJsonMultiPolygon& polygons) {
    for (const std::vector<std::vector<glm::dvec3>>& polygon :
         polygons.coordinates) {
      for (const std::vector<glm::dvec3>& ring : polygon) {
        for (const glm::dvec3& point : ring) {
          builder.expandToIncludePosition(
              Cartographic::fromDegrees(point.x, point.y));
        }
      }
    }
  }
  void operator()(const GeoJsonFeature&) {}
  void operator()(const GeoJsonFeatureCollection&) {}
  void operator()(const GeoJsonGeometryCollection&) {}
};

void addPrimitivesToData(
    const GeoJsonObject* pGeoJsonObject,
    std::vector<QuadtreeGeometryData>& data,
    BoundingRegionBuilder& documentRegionBuilder,
    const VectorStyle& style);

struct GeoJsonChildVisitor {
  std::vector<QuadtreeGeometryData>& data;
  BoundingRegionBuilder& documentRegionBuilder;
  const VectorStyle& style;

  void operator()(const GeoJsonFeature& feature) {
    if (feature.geometry) {
      const std::optional<VectorStyle>& geometryStyle =
          feature.geometry->getStyle();
      const std::optional<VectorStyle>& featureStyle =
          geometryStyle ? geometryStyle : feature.style;
      addPrimitivesToData(
          feature.geometry.get(),
          data,
          documentRegionBuilder,
          featureStyle ? *featureStyle : style);
    }
  }

  void operator()(const GeoJsonFeatureCollection& collection) {
    for (const GeoJsonObject& feature : collection.features) {
      const GeoJsonFeature* pFeature = feature.getIf<GeoJsonFeature>();
      if (pFeature && pFeature->geometry) {
        const std::optional<VectorStyle>& geometryStyle = feature.getStyle();
        const std::optional<VectorStyle>& featureStyle =
            geometryStyle ? geometryStyle : pFeature->style;
        const std::optional<VectorStyle>& collectionStyle =
            featureStyle ? featureStyle : pFeature->style;
        addPrimitivesToData(
            pFeature->geometry.get(),
            data,
            documentRegionBuilder,
            collectionStyle ? *collectionStyle : style);
      }
    }
  }

  void operator()(const GeoJsonGeometryCollection& collection) {
    for (const GeoJsonObject& geometry : collection.geometries) {
      const std::optional<VectorStyle>& childStyle = geometry.getStyle();
      const std::optional<VectorStyle>& useStyle =
          childStyle ? childStyle : collection.style;
      addPrimitivesToData(
          &geometry,
          data,
          documentRegionBuilder,
          useStyle ? *useStyle : style);
    }
  }

  void operator()(const auto& /*catchAll*/) {}
};

void addPrimitivesToData(
    const GeoJsonObject* geoJsonObject,
    std::vector<QuadtreeGeometryData>& data,
    BoundingRegionBuilder& documentRegionBuilder,
    const VectorStyle& style) {
  BoundingRegionBuilder thisBuilder;
  std::visit(
      GlobeRectangleFromObjectVisitor{thisBuilder},
      geoJsonObject->value);
  GlobeRectangle rect = thisBuilder.toGlobeRectangle();
  documentRegionBuilder.expandToIncludeGlobeRectangle(rect);
  data.emplace_back(
      QuadtreeGeometryData{geoJsonObject, &style, std::move(rect)});

  std::visit(
      GeoJsonChildVisitor{data, documentRegionBuilder, style},
      geoJsonObject->value);
}

const uint32_t DEPTH_LIMIT = 8;
uint32_t buildQuadtreeNode(
    Quadtree& tree,
    const QuadtreeTilingScheme& tilingScheme,
    const GlobeRectangle& rectangle,
    std::vector<uint32_t>::iterator begin,
    std::vector<uint32_t>::iterator end,
    QuadtreeTileID tileId) {
  if (begin == end) {
    return 0;
  }

  uint32_t resultId = (uint32_t)tree.nodes.size();
  tree.nodes.emplace_back(rectangle);

  uint32_t indicesBegin = (uint32_t)tree.dataIndices.size();
  tree.dataIndices.insert(tree.dataIndices.end(), begin, end);

  tree.dataNodeIndicesBegin.emplace_back(indicesBegin);

  if (begin + 1 == end || tileId.level >= DEPTH_LIMIT ||
      std::equal(begin + 1, end, begin)) {
    return resultId;
  }

  const CesiumGeometry::QuadtreeTileID southWestTile(
      tileId.level + 1,
      tileId.x * 2,
      tileId.y * 2);
  const CesiumGeometry::QuadtreeTileID southEastTile(
      southWestTile.level,
      southWestTile.x + 1,
      southWestTile.y);
  const CesiumGeometry::QuadtreeTileID northWestTile(
      southWestTile.level,
      southWestTile.x,
      southWestTile.y + 1);
  const CesiumGeometry::QuadtreeTileID northEastTile(
      southWestTile.level,
      southWestTile.x + 1,
      southWestTile.y + 1);

  const GlobeRectangle southWestRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(southWestTile));
  tree.nodes[resultId].children[0][0] = buildQuadtreeNode(
      tree,
      tilingScheme,
      southWestRect,
      begin,
      std::partition(
          begin,
          end,
          [&southWestRect, &data = tree.data](uint32_t idx) {
            return data[idx]
                .rectangle.computeIntersection(southWestRect)
                .has_value();
          }),
      southWestTile);

  const GlobeRectangle southEastRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(southEastTile));
  tree.nodes[resultId].children[0][1] = buildQuadtreeNode(
      tree,
      tilingScheme,
      southEastRect,
      begin,
      std::partition(
          begin,
          end,
          [&southEastRect, &data = tree.data](uint32_t idx) {
            return data[idx]
                .rectangle.computeIntersection(southEastRect)
                .has_value();
          }),
      southEastTile);

  const GlobeRectangle northWestRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(northWestTile));
  tree.nodes[resultId].children[1][0] = buildQuadtreeNode(
      tree,
      tilingScheme,
      northWestRect,
      begin,
      std::partition(
          begin,
          end,
          [&northWestRect, &data = tree.data](uint32_t idx) {
            return data[idx]
                .rectangle.computeIntersection(northWestRect)
                .has_value();
          }),
      northWestTile);

  const GlobeRectangle northEastRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(northEastTile));
  tree.nodes[resultId].children[1][1] = buildQuadtreeNode(
      tree,
      tilingScheme,
      northEastRect,
      begin,
      std::partition(
          begin,
          end,
          [&northEastRect, &data = tree.data](uint32_t idx) {
            return data[idx]
                .rectangle.computeIntersection(northEastRect)
                .has_value();
          }),
      northEastTile);

  return resultId;
}

Quadtree buildQuadtree(
    const std::shared_ptr<GeoJsonDocument>& document,
    const VectorStyle& defaultStyle) {
  BoundingRegionBuilder builder;
  std::vector<QuadtreeGeometryData> data;
  const std::optional<VectorStyle>& rootObjectStyle =
      document->rootObject.getStyle();
  addPrimitivesToData(
      &document->rootObject,
      data,
      builder,
      rootObjectStyle ? *rootObjectStyle : defaultStyle);

  Quadtree tree{
      builder.toGlobeRectangle(),
      0,
      std::vector<QuadtreeNode>(),
      std::move(data),
      std::vector<uint32_t>(),
      std::vector<uint32_t>()};

  std::vector<uint32_t> dataIndices;
  dataIndices.reserve(tree.data.size());
  for (size_t i = 0; i < tree.data.size(); i++) {
    dataIndices.emplace_back((uint32_t)i);
  }

  const QuadtreeTilingScheme tilingScheme(
      tree.rectangle.toSimpleRectangle(),
      1,
      1);

  tree.rootId = buildQuadtreeNode(
      tree,
      tilingScheme,
      tree.rectangle,
      dataIndices.begin(),
      dataIndices.end(),
      QuadtreeTileID(0, 0, 0));
  // Add last entry so [i + 1] is always valid
  tree.dataNodeIndicesBegin.emplace_back((uint32_t)tree.dataIndices.size() - 1);

  return tree;
}

void rasterizeQuadtreeNode(
    const Quadtree& tree,
    uint32_t nodeId,
    const GlobeRectangle& rectangle,
    VectorRasterizer& rasterizer,
    std::vector<bool>& primitivesRendered) {
  const QuadtreeNode& node = tree.nodes[nodeId];
  // If this node has no children, or if it is entirely within the target
  // rectangle, let's rasterize this node's contents and not any children.
  if (!node.anyChildren() ||
      (rectangle.contains(node.rectangle.getSouthwest()) &&
       rectangle.contains(node.rectangle.getNortheast()))) {
    for (uint32_t i = tree.dataNodeIndicesBegin[nodeId];
         i < tree.dataNodeIndicesBegin[nodeId + 1];
         i++) {
      const uint32_t dataIdx = tree.dataIndices[i];
      if (primitivesRendered[dataIdx]) {
        continue;
      }
      primitivesRendered[dataIdx] = true;
      const QuadtreeGeometryData& data = tree.data[dataIdx];
      rasterizer.drawGeoJsonObject(*data.pObject, *data.pStyle);
    }
  } else {
    for (size_t i = 0; i < 2; i++) {
      for (size_t j = 0; j < 2; j++) {
        if (rectangle.computeIntersection(
                tree.nodes[node.children[i][j]].rectangle)) {
          rasterizeQuadtreeNode(
              tree,
              node.children[i][j],
              rectangle,
              rasterizer,
              primitivesRendered);
        }
      }
    }
  }
}

void rasterizeVectorData(
    LoadedRasterOverlayImage& result,
    const GlobeRectangle& rectangle,
    const Quadtree& tree,
    const Ellipsoid& ellipsoid) {
  // Keeps track of primitives that have already been rendered to avoid
  // re-drawing the same primitives that appear in multiple quadtree nodes.
  std::vector<bool> primitivesRendered(tree.data.size(), false);
  for (size_t i = 0;
       i < std::max(result.pImage->mipPositions.size(), (size_t)1);
       i++) {
    primitivesRendered.assign(primitivesRendered.size(), false);

    VectorRasterizer rasterizer(
        rectangle,
        result.pImage,
        (uint32_t)i,
        ellipsoid);
    rasterizeQuadtreeNode(
        tree,
        tree.rootId,
        rectangle,
        rasterizer,
        primitivesRendered);
    rasterizer.finalize();
  }
}
} // namespace

class CESIUMRASTEROVERLAYS_API GeoJsonDocumentRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::shared_ptr<GeoJsonDocument> _pDocument;
  VectorStyle _defaultStyle;
  Quadtree _tree;
  Ellipsoid _ellipsoid;
  uint32_t _mipLevels;

public:
  GeoJsonDocumentRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const GeoJsonDocumentRasterOverlayOptions& options,
      std::shared_ptr<CesiumVectorData::GeoJsonDocument>&& pDocument)
      : RasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            pCreditSystem,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            GeographicProjection(options.ellipsoid),
            projectRectangleSimple(
                GeographicProjection(options.ellipsoid),
                GlobeRectangle::MAXIMUM)),
        _pDocument(std::move(pDocument)),
        _defaultStyle(options.defaultStyle),
        _tree(),
        _ellipsoid(options.ellipsoid),
        _mipLevels(options.mipLevels) {
    CESIUM_ASSERT(this->_pDocument);
    this->_tree = buildQuadtree(this->_pDocument, this->_defaultStyle);
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    // Choose the texture size according to the geometry screen size and raster
    // SSE, but no larger than the maximum texture size.
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::ivec2 textureSize = glm::min(
        glm::ivec2(
            overlayTile.getTargetScreenPixels() /
            options.maximumScreenSpaceError),
        glm::ivec2(options.maximumTextureSize));

    return this->getAsyncSystem().runInWorkerThread(
        [&tree = this->_tree,
         _ellipsoid = this->_ellipsoid,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle(),
         textureSize,
         mipLevels = this->_mipLevels]() -> LoadedRasterOverlayImage {
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage result;
          result.rectangle = rectangle;

          if (!tileRectangle.computeIntersection(tree.rectangle)) {
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
            if (mipLevels == 0) {
              result.pImage->pixelData.resize(
                  (size_t)(result.pImage->width * result.pImage->height *
                           result.pImage->channels *
                           result.pImage->bytesPerChannel),
                  std::byte{0});
            } else {
              size_t totalSize = 0;
              result.pImage->mipPositions.reserve((size_t)mipLevels);
              for (uint32_t i = 0; i < mipLevels; i++) {
                const int32_t width = std::max(textureSize.x >> i, 1);
                const int32_t height = std::max(textureSize.y >> i, 1);
                result.pImage->mipPositions.emplace_back(
                    CesiumGltf::ImageAssetMipPosition{
                        totalSize,
                        (size_t)(width * height * result.pImage->channels *
                                 result.pImage->bytesPerChannel)});
                totalSize += result.pImage->mipPositions[i].byteSize;
              }
              result.pImage->pixelData.resize(totalSize, std::byte{0});
            }
            rasterizeVectorData(result, tileRectangle, tree, _ellipsoid);
          }

          return result;
        });
  }
};

GeoJsonDocumentRasterOverlay::GeoJsonDocumentRasterOverlay(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& name,
    const std::shared_ptr<GeoJsonDocument>& document,
    const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _documentFuture(asyncSystem.createResolvedFuture(
          std::shared_ptr<GeoJsonDocument>(document))),
      _options(vectorOverlayOptions) {}

GeoJsonDocumentRasterOverlay::GeoJsonDocumentRasterOverlay(
    const std::string& name,
    CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>&&
        documentFuture,
    const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _documentFuture(std::move(documentFuture)),
      _options(vectorOverlayOptions) {}

GeoJsonDocumentRasterOverlay::~GeoJsonDocumentRasterOverlay() = default;

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
GeoJsonDocumentRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  pOwner = pOwner ? pOwner : this;

  return std::move(
             const_cast<GeoJsonDocumentRasterOverlay*>(this)->_documentFuture)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           pLogger,
           options =
               this->_options](std::shared_ptr<GeoJsonDocument>&& pDocument)
              -> CreateTileProviderResult {
            if (!pDocument) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::Unknown,
                  .pRequest = nullptr,
                  .message = "GeoJSON document failed to load."});
            }

            return IntrusivePointer<RasterOverlayTileProvider>(
                new GeoJsonDocumentRasterOverlayTileProvider(
                    pOwner,
                    asyncSystem,
                    pAssetAccessor,
                    pCreditSystem,
                    pPrepareRendererResources,
                    pLogger,
                    options,
                    std::move(pDocument)));
          });
}

} // namespace CesiumRasterOverlays
