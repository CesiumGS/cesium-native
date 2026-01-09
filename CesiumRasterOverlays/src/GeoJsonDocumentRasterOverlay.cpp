#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorRasterizer.h>
#include <CesiumVectorData/VectorStyle.h>

#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/spdlog.h>

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

// We won't generate any quadtree nodes past this depth.
const uint32_t DEPTH_LIMIT = 30;

// The number of primitives in a node before it splits.
const size_t NODE_COUNT_THRESHOLD = 32;

// The exact texture size depends on the geometric error of the tile it's
// attached to, among other things. We use a reasonable value for the smallest
// texture size as this will be the case where the pixel width of a
// line/outline will have the most impact on the size of a geometry primitive.
// const glm::ivec2 BASE_TEXTURE_SIZE(64, 64);

namespace {
// Line-line intersection algorithm from "Tricks of the Windows Game Programming
// Gurus" by Andre Lamothe
bool lineLineIntersection2D(
    const glm::dvec3& a0,
    const glm::dvec3& a1,
    const glm::dvec3& b0,
    const glm::dvec3& b1) {
  const double s1X = a1.x - a0.x;
  const double s1Y = a1.y - a0.y;

  const double s2X = b1.x - b0.x;
  const double s2Y = b1.y - b0.y;

  const double s =
      (-s1Y * (a0.x - b0.x) + s1X * (a0.y - b0.y)) / (-s2X * s1Y + s1X * s2Y);
  const double t =
      (s2X * (a0.y - b0.y) - s2Y * (a0.x - b0.x)) / (-s2X * s1Y + s1X * s2Y);

  return s >= 0.0 && s <= 1.0 && t >= 0.0 && t <= 1.0;
}

bool lineRectangleIntersection2D(
    const glm::dvec3& a0,
    const glm::dvec3& a1,
    const GlobeRectangle& rect) {
  const glm::dvec3 northWest(rect.getWest(), rect.getNorth(), 0.0);
  const glm::dvec3 northEast(rect.getEast(), rect.getNorth(), 0.0);
  const glm::dvec3 southWest(rect.getWest(), rect.getSouth(), 0.0);
  const glm::dvec3 southEast(rect.getEast(), rect.getSouth(), 0.0);

  return lineLineIntersection2D(a0, a1, northWest, northEast) ||
         lineLineIntersection2D(a0, a1, northEast, southEast) ||
         lineLineIntersection2D(a0, a1, southWest, southEast) ||
         lineLineIntersection2D(a0, a1, northWest, southWest);
}

bool lineStringRectangleIntersection2D(
    const std::vector<glm::dvec3>& points,
    const GlobeRectangle& rect) {
  if (points.size() < 2) {
    return false;
  }

  for (size_t i = 0; i < points.size() - 1; i++) {
    if (rect.contains(Cartographic(points[i].x, points[i].y, points[i].z)) &&
        rect.contains(
            Cartographic(points[i + 1].x, points[i + 1].y, points[i + 1].z))) {
      return true;
    }
    if (lineRectangleIntersection2D(points[i], points[i + 1], rect)) {
      return true;
    }
  }

  return false;
}
} // namespace

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

  /**
   * @brief Calculates the size of the bounding rectangle for this geometry when
   * rendered on a tile with the given bounds and texture size.
   *
   * For a tile that has no line width or where line width is specified in
   * `LineWidthMode::Meters`, this will be equivalent to `rectangle`. For tiles
   * where the line width is specified in `LineWidthMode::Pixels`, this result
   * can change depending on texture size and bounds.
   *
   * @param tileBounds The bounding box of this tile.
   * @param textureSize The size of the texture this tile will be rasterized
   * into.
   */
  GlobeRectangle calculateBoundingRectangleForTileSize(
      const GlobeRectangle& tileBounds,
      const glm::ivec2& textureSize) {
    LineStyle activeLineStyle;
    if (this->pObject->isType<GeoJsonPolygon>() ||
        this->pObject->isType<GeoJsonMultiPolygon>()) {
      if (!this->pStyle->polygon.outline) {
        return this->rectangle;
      }

      activeLineStyle = *this->pStyle->polygon.outline;
    } else {
      activeLineStyle = this->pStyle->line;
    }

    // We already accounted for meters when building the rectangle in the first
    // place.
    if (activeLineStyle.widthMode == LineWidthMode::Meters) {
      return this->rectangle;
    }

    const double longPerPixel =
        tileBounds.computeWidth() / static_cast<double>(textureSize.x);
    const double latPerPixel =
        tileBounds.computeHeight() / static_cast<double>(textureSize.y);
    const double halfX = longPerPixel * activeLineStyle.width * 0.5;
    const double halfY = latPerPixel * activeLineStyle.width * 0.5;

    return GlobeRectangle(
        this->rectangle.getWest() - halfX,
        this->rectangle.getSouth() - halfY,
        this->rectangle.getEast() + halfX,
        this->rectangle.getNorth() + halfY);
  }

  bool testRectIntersection2D(const GlobeRectangle& rect) {
    struct RectIntersectionVisitor {
      const GlobeRectangle& ourAabb;
      const GlobeRectangle& testAabb;

      bool operator()(const GeoJsonLineString& line) {
        return lineStringRectangleIntersection2D(line.coordinates, testAabb);
      }
      bool operator()(const GeoJsonMultiLineString& line) {
        for (const std::vector<glm::dvec3>& coordinates : line.coordinates) {
          if (lineStringRectangleIntersection2D(coordinates, testAabb)) {
            return true;
          }
        }

        return false;
      }
      bool operator()(const GeoJsonPolygon& /*polygon*/) {
        return this->ourAabb.computeIntersection(testAabb).has_value();
      }
      bool operator()(const GeoJsonMultiPolygon& /*polygons*/) {
        return this->ourAabb.computeIntersection(testAabb).has_value();
      }
      bool operator()(const GeoJsonFeature&) { return false; }
      bool operator()(const GeoJsonFeatureCollection&) { return false; }
      bool operator()(const GeoJsonGeometryCollection&) { return false; }
      // While we could calculate a bounding box for a point just fine, they are
      // not rendered by the raster overlay, so there's no need.
      bool operator()(const GeoJsonPoint&) { return false; }
      bool operator()(const GeoJsonMultiPoint&) { return false; }
    };

    return this->pObject->visit(RectIntersectionVisitor{this->rectangle, rect});
  }
};

struct QuadtreeNode {
  QuadtreeTileID tileId;
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
  /**
   * Indices into `tree.data` for every primitive that this node contains.
   */
  std::vector<uint32_t> data;

  QuadtreeNode(QuadtreeTileID tileId_, const GlobeRectangle& rectangle_)
      : tileId(tileId_), rectangle(rectangle_) {}

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
};

struct GlobeRectangleFromObjectVisitor {
  BoundingRegionBuilder& builder;
  const VectorStyle& style;
  const Ellipsoid& ellipsoid;

  void visitWithLineWidth(
      const std::vector<glm::dvec3>& coordinates,
      const std::optional<LineStyle>& lineStyle) {
    // For geometry where the line width is specified in meters, we can include
    // that in the bounding region calculation up front.
    if (lineStyle && lineStyle->widthMode == LineWidthMode::Meters) {
      const double halfWidth =
          (lineStyle->width / ellipsoid.getRadii().x) / 2.0;
      for (const glm::dvec3& point : coordinates) {
        builder.expandToIncludePosition(Cartographic::fromDegrees(
            point.x - halfWidth,
            point.y - halfWidth));
        builder.expandToIncludePosition(Cartographic::fromDegrees(
            point.x + halfWidth,
            point.y + halfWidth));
      }
    } else {
      for (const glm::dvec3& point : coordinates) {
        builder.expandToIncludePosition(
            Cartographic::fromDegrees(point.x, point.y));
      }
    }
  }
  void operator()(const GeoJsonLineString& line) {
    visitWithLineWidth(line.coordinates, style.line);
  }
  void operator()(const GeoJsonMultiLineString& lines) {
    for (const std::vector<glm::dvec3>& line : lines.coordinates) {
      visitWithLineWidth(line, style.line);
    }
  }
  void operator()(const GeoJsonPolygon& polygon) {
    for (const std::vector<glm::dvec3>& ring : polygon.coordinates) {
      visitWithLineWidth(ring, style.polygon.outline);
    }
  }
  void operator()(const GeoJsonMultiPolygon& polygons) {
    for (const std::vector<std::vector<glm::dvec3>>& polygon :
         polygons.coordinates) {
      for (const std::vector<glm::dvec3>& ring : polygon) {
        visitWithLineWidth(ring, style.polygon.outline);
      }
    }
  }
  void operator()(const GeoJsonFeature&) {}
  void operator()(const GeoJsonFeatureCollection&) {}
  void operator()(const GeoJsonGeometryCollection&) {}
  // While we could calculate a bounding box for a point just fine, they are not
  // rendered by the raster overlay, so there's no need.
  void operator()(const GeoJsonPoint&) {}
  void operator()(const GeoJsonMultiPoint&) {}
};

void addPrimitivesToData(
    const GeoJsonObject* pGeoJsonObject,
    std::vector<QuadtreeGeometryData>& data,
    BoundingRegionBuilder& documentRegionBuilder,
    const VectorStyle& style,
    const Ellipsoid& ellipsoid);

struct GeoJsonChildVisitor {
  std::vector<QuadtreeGeometryData>& data;
  BoundingRegionBuilder& documentRegionBuilder;
  const VectorStyle& style;
  const Ellipsoid& ellipsoid;

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
          featureStyle ? *featureStyle : style,
          ellipsoid);
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
            collectionStyle ? *collectionStyle : style,
            ellipsoid);
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
          useStyle ? *useStyle : style,
          ellipsoid);
    }
  }

  void operator()(const auto& /*catchAll*/) {}
};

void addPrimitivesToData(
    const GeoJsonObject* geoJsonObject,
    std::vector<QuadtreeGeometryData>& data,
    BoundingRegionBuilder& documentRegionBuilder,
    const VectorStyle& style,
    const Ellipsoid& ellipsoid) {
  BoundingRegionBuilder thisBuilder;
  std::visit(
      GlobeRectangleFromObjectVisitor{thisBuilder, style, ellipsoid},
      geoJsonObject->value);
  GlobeRectangle rect = thisBuilder.toGlobeRectangle();
  // Points and MultiPoints, as well as Features, FeatureCollections, and
  // GeometryCollections will return a zero-size bounding box. For the first
  // two, this is because they are not rasterized by this overlay. For the rest,
  // though they may contain geometry, they do not themselves have anything to
  // render. We can save some effort by ignoring them all now.
  if (rect.computeWidth() != 0.0 || rect.computeHeight() != 0.0) {
    QuadtreeGeometryData primitive{geoJsonObject, &style, std::move(rect)};
    documentRegionBuilder.expandToIncludeGlobeRectangle(rect);
    data.emplace_back(primitive);
  }

  std::visit(
      GeoJsonChildVisitor{data, documentRegionBuilder, style, ellipsoid},
      geoJsonObject->value);
}

void addToQuadtree(
    Quadtree& tree,
    uint32_t nodeIdx,
    const QuadtreeTilingScheme& tilingScheme,
    uint32_t dataIndex);

uint32_t splitNodeContentsForRectangle(
    Quadtree& tree,
    uint32_t parentNodeIdx,
    const QuadtreeTilingScheme& tilingScheme,
    const GlobeRectangle& childRectangle,
    QuadtreeTileID childId) {
  const uint32_t nodeId = (uint32_t)tree.nodes.size();
  tree.nodes.emplace_back(childId, childRectangle);

  for (uint32_t dataIdx : tree.nodes[parentNodeIdx].data) {
    if (tree.data[dataIdx].testRectIntersection2D(childRectangle)) {
      addToQuadtree(tree, nodeId, tilingScheme, dataIdx);
    }
  }

  return nodeId;
}

void addToQuadtree(
    Quadtree& tree,
    uint32_t nodeIdx,
    const QuadtreeTilingScheme& tilingScheme,
    uint32_t dataIndex) {
  tree.nodes[nodeIdx].data.push_back(dataIndex);

  if (tree.nodes[nodeIdx].data.size() < NODE_COUNT_THRESHOLD ||
      tree.nodes[nodeIdx].tileId.level >= DEPTH_LIMIT) {
    return;
  }

  if (tree.nodes[nodeIdx].anyChildren()) {
    // This node already has children, we can just add to the existing children.
    for (size_t i = 0; i < 2; i++) {
      for (size_t j = 0; j < 2; j++) {
        addToQuadtree(
            tree,
            tree.nodes[nodeIdx].children[i][j],
            tilingScheme,
            dataIndex);
      }
    }

    return;
  }

  // At this point adding more nodes will not help us be any more efficient.
  if(tree.nodes.size() >= tree.data.size()) {
    return;
  }

  // We need to split the node.
  const CesiumGeometry::QuadtreeTileID southWestTile(
      tree.nodes[nodeIdx].tileId.level + 1,
      tree.nodes[nodeIdx].tileId.x * 2,
      tree.nodes[nodeIdx].tileId.y * 2);
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
  const GlobeRectangle southEastRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(southEastTile));
  const GlobeRectangle northWestRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(northWestTile));
  const GlobeRectangle northEastRect = GlobeRectangle::fromRectangleRadians(
      tilingScheme.tileToRectangle(northEastTile));

  tree.nodes[nodeIdx].children[0][0] = splitNodeContentsForRectangle(
      tree,
      nodeIdx,
      tilingScheme,
      southWestRect,
      southWestTile);
  tree.nodes[nodeIdx].children[0][1] = splitNodeContentsForRectangle(
      tree,
      nodeIdx,
      tilingScheme,
      southEastRect,
      southEastTile);
  tree.nodes[nodeIdx].children[1][0] = splitNodeContentsForRectangle(
      tree,
      nodeIdx,
      tilingScheme,
      northWestRect,
      northWestTile);
  tree.nodes[nodeIdx].children[1][1] = splitNodeContentsForRectangle(
      tree,
      nodeIdx,
      tilingScheme,
      northEastRect,
      northEastTile);
}

Quadtree buildQuadtree(
    const std::shared_ptr<GeoJsonDocument>& document,
    const VectorStyle& defaultStyle,
    const Ellipsoid& ellipsoid) {
  BoundingRegionBuilder builder;
  std::vector<QuadtreeGeometryData> data;
  const std::optional<VectorStyle>& rootObjectStyle =
      document->rootObject.getStyle();
  addPrimitivesToData(
      &document->rootObject,
      data,
      builder,
      rootObjectStyle ? *rootObjectStyle : defaultStyle,
      ellipsoid);

  Quadtree tree{
      builder.toGlobeRectangle(),
      0,
      std::vector<QuadtreeNode>(),
      std::move(data)};

  const QuadtreeTilingScheme tilingScheme(
      tree.rectangle.toSimpleRectangle(),
      1,
      1);

  tree.rootId = 0;
  tree.nodes.emplace_back(QuadtreeTileID(0, 0, 0), tree.rectangle);

  for (uint32_t i = 0; i < static_cast<uint32_t>(tree.data.size()); i++) {
    addToQuadtree(tree, tree.rootId, tilingScheme, i);
  }

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
    for (uint32_t dataIdx : node.data) {
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
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const GeoJsonDocumentRasterOverlayOptions& geoJsonOptions,
      std::shared_ptr<CesiumVectorData::GeoJsonDocument>&& pDocument)
      : RasterOverlayTileProvider(
            pCreator,
            parameters,
            GeographicProjection(geoJsonOptions.ellipsoid),
            projectRectangleSimple(
                GeographicProjection(geoJsonOptions.ellipsoid),
                GlobeRectangle::MAXIMUM)),
        _pDocument(std::move(pDocument)),
        _defaultStyle(geoJsonOptions.defaultStyle),
        _tree(),
        _ellipsoid(geoJsonOptions.ellipsoid),
        _mipLevels(geoJsonOptions.mipLevels) {
    CESIUM_ASSERT(this->_pDocument);
    this->_tree = buildQuadtree(
        this->_pDocument,
        this->_defaultStyle,
        geoJsonOptions.ellipsoid);
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

          if (false && !tileRectangle.computeIntersection(tree.rectangle)) {
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
    const CreateRasterOverlayTileProviderParameters& parameters) const {

  IntrusivePointer<const GeoJsonDocumentRasterOverlay> thiz = this;

  return std::move(
             const_cast<GeoJsonDocumentRasterOverlay*>(this)->_documentFuture)
      .thenInMainThread(
          [thiz, parameters](std::shared_ptr<GeoJsonDocument>&& pDocument)
              -> CreateTileProviderResult {
            if (!pDocument) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::Unknown,
                  .pRequest = nullptr,
                  .message = "GeoJSON document failed to load."});
            }

            return IntrusivePointer<RasterOverlayTileProvider>(
                new GeoJsonDocumentRasterOverlayTileProvider(
                    thiz,
                    parameters,
                    thiz->_options,
                    std::move(pDocument)));
          });
}

} // namespace CesiumRasterOverlays
