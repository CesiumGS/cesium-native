#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorRasterizer.h>
#include <CesiumVectorData/VectorStyle.h>
#include <CesiumVectorOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumVectorOverlays/Library.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumVectorData;
using namespace CesiumRasterOverlays;

// We won't generate any quadtree nodes past this depth.
const uint32_t DEPTH_LIMIT = 8;

namespace CesiumVectorOverlays {

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
   * @brief The index into `Quadtree::styles` of the `VectorStyle` to apply to
   * this geometry object.
   */
  uint32_t styleIndex;
  /**
   * @brief The bounding rectangle encompassing this geometry.
   */
  GlobeRectangle rectangle;
  /**
   * The maximum width of all lines or outlines for this geometry with
   * `LineWidthMode::Pixels`.
   */
  double maxLineWidthPixels = 0.0;

  /**
   * @brief Calculates the size of the bounding rectangle for this geometry when
   * rendered on a tile with the given bounds and texture size.
   *
   * For a tile that has no line width or where line width is specified in
   * `LineWidthMode::Meters`, this will be equivalent to `rectangle`. For tiles
   * where the line width is specified in `LineWidthMode::Pixels`, this result
   * can change depending on texture size and bounds.
   *
   * @param style The style of this geometry, `Quadtree::styles[styleIndex]`.
   * @param tileBounds The bounding box of this tile.
   * @param textureSize The size of the texture this tile will be rasterized
   * into.
   */
  GlobeRectangle calculateBoundingRectangleForTileSize(
      const VectorStyle& style,
      const GlobeRectangle& tileBounds,
      const glm::ivec2& textureSize) {
    LineStyle activeLineStyle;
    if (this->pObject->isType<GeoJsonPolygon>() ||
        this->pObject->isType<GeoJsonMultiPolygon>()) {
      if (!style.polygon.outline) {
        return this->rectangle;
      }

      activeLineStyle = *style.polygon.outline;
    } else if (
        this->pObject->isType<GeoJsonPoint>() ||
        this->pObject->isType<GeoJsonMultiPoint>()) {
      // A point's pixel footprint is its radius (plus its outline width) in
      // every direction. Encode that as an equivalent line width (the full
      // diameter) so the shared pixel padding math below applies to it too.
      activeLineStyle.width = 2.0 * style.point.radius;
      if (style.point.outline) {
        activeLineStyle.width += style.point.outline->width;
      }
      activeLineStyle.widthMode = LineWidthMode::Pixels;
    } else {
      activeLineStyle = style.line;
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

    double newWest = this->rectangle.getWest() - halfX;
    if (newWest < -180.0) {
      newWest += 360.0;
    }
    double newEast = this->rectangle.getEast() + halfX;
    if (newEast > 180.0) {
      newEast -= 360.0;
    }

    BoundingRegionBuilder builder;
    builder.expandToIncludeGlobeRectangle(this->rectangle);
    builder.expandToIncludePosition(
        Cartographic(newWest, this->rectangle.getSouth() - halfY));
    builder.expandToIncludePosition(
        Cartographic(newEast, this->rectangle.getNorth() + halfY));

    return builder.toGlobeRectangle();
  }
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
  /**
   * The maximum width of all lines or outlines in this node with
   * `LineWidthMode::Pixels`.
   */
  double maxLineWidthPixels = 0.0;

  QuadtreeNode(const GlobeRectangle& rectangle_) : rectangle(rectangle_) {}

  /**
   * @brief Returns `true` if this node has any children.
   */
  bool anyChildren() const {
    return children[0][0] != 0 && children[0][1] != 0 && children[1][0] != 0 &&
           children[1][1] != 0;
  }

  GlobeRectangle
  getRectangleScaledWithPixelSize(const glm::dvec2& halfPixelSize) const {
    if (this->maxLineWidthPixels == 0.0) {
      return this->rectangle;
    }

    const glm::dvec2 scaledHalfPixelSize =
        halfPixelSize * this->maxLineWidthPixels;

    return GlobeRectangle(
        this->rectangle.getWest() - scaledHalfPixelSize.x,
        this->rectangle.getSouth() - scaledHalfPixelSize.y,
        this->rectangle.getEast() + scaledHalfPixelSize.x,
        this->rectangle.getNorth() + scaledHalfPixelSize.y);
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
  /**
   * @brief Copies of every style used by `data`. They are copied on the main
   * thread when the tree is built, so the styles in the document may be
   * changed afterward without affecting rasterization on worker threads.
   */
  std::vector<VectorStyle> styles;
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

struct RectangleAndLineWidthFromObjectVisitor {
  std::optional<GlobeRectangle>& rect;
  double& maxLineWidthPixels;
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
        if (!rect) {
          rect = GlobeRectangle(
              std::min(
                  Math::degreesToRadians(point.x) - halfWidth,
                  -Math::OnePi),
              Math::degreesToRadians(point.y) - halfWidth,
              std::max(
                  Math::degreesToRadians(point.x) + halfWidth,
                  Math::OnePi),
              Math::degreesToRadians(point.y) + halfWidth);
        } else {
          rect->setWest(std::max(
              std::min(
                  rect->getWest(),
                  Math::degreesToRadians(point.x) - halfWidth),
              -Math::OnePi));
          rect->setSouth(std::min(
              rect->getSouth(),
              Math::degreesToRadians(point.y) - halfWidth));
          rect->setEast(std::min(
              std::max(
                  rect->getEast(),
                  Math::degreesToRadians(point.x) + halfWidth),
              Math::OnePi));
          rect->setNorth(std::max(
              rect->getNorth(),
              Math::degreesToRadians(point.y) + halfWidth));
        }
      }
    } else {
      if (lineStyle && lineStyle->widthMode == LineWidthMode::Pixels) {
        this->maxLineWidthPixels =
            std::max(this->maxLineWidthPixels, lineStyle->width);
      }
      for (const glm::dvec3& point : coordinates) {
        if (!rect) {
          rect = GlobeRectangle(
              Math::degreesToRadians(point.x),
              Math::degreesToRadians(point.y),
              Math::degreesToRadians(point.x),
              Math::degreesToRadians(point.y));
        } else {
          rect->setWest(
              std::min(rect->getWest(), Math::degreesToRadians(point.x)));
          rect->setSouth(
              std::min(rect->getSouth(), Math::degreesToRadians(point.y)));
          rect->setEast(
              std::max(rect->getEast(), Math::degreesToRadians(point.x)));
          rect->setNorth(
              std::max(rect->getNorth(), Math::degreesToRadians(point.y)));
        }
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
  void visitPoint(const glm::dvec3& point) {
    // A point extends `radius` pixels in every direction from its center, plus
    // its outline width if it is outlined. We track the full diameter as an
    // equivalent pixel "line width" so the tile bounds get padded the same way
    // they are for lines and polygon outlines.
    double pointExtentPixels = 2.0 * style.point.radius;
    if (style.point.outline) {
      pointExtentPixels += style.point.outline->width;
    }
    this->maxLineWidthPixels =
        std::max(this->maxLineWidthPixels, pointExtentPixels);

    const double longitude = Math::degreesToRadians(point.x);
    const double latitude = Math::degreesToRadians(point.y);
    if (!rect) {
      rect = GlobeRectangle(longitude, latitude, longitude, latitude);
    } else {
      rect->setWest(std::min(rect->getWest(), longitude));
      rect->setSouth(std::min(rect->getSouth(), latitude));
      rect->setEast(std::max(rect->getEast(), longitude));
      rect->setNorth(std::max(rect->getNorth(), latitude));
    }
  }
  void operator()(const GeoJsonPoint& point) { visitPoint(point.coordinates); }
  void operator()(const GeoJsonMultiPoint& points) {
    for (const glm::dvec3& point : points.coordinates) {
      visitPoint(point);
    }
  }
};

/**
 * @brief A geometry object to render and the style it resolved to, before its
 * bounds have been computed.
 */
struct ResolvedPrimitive {
  const GeoJsonObject* pObject;
  uint32_t styleIndex;
};

/**
 * @brief The geometry objects of a document with copies of their resolved
 * styles.
 *
 * This is created on the main thread, where it's safe to read the document's
 * styles, and is then used to build a `Quadtree` on a worker thread.
 */
struct ResolvedDocument {
  std::vector<ResolvedPrimitive> primitives;
  std::vector<VectorStyle> styles;
};

/**
 * @brief Walks a document and resolves the style of each geometry object: the
 * geometry's own style, then its feature's, then its collection's, then the
 * default style.
 */
class StyleResolver {
public:
  explicit StyleResolver(ResolvedDocument& result) : _result(result) {}

  void addObject(const GeoJsonObject& object, const VectorStyle& style) {
    // Features and collections have nothing to render themselves; only their
    // children do.
    if (!object.isType<GeoJsonFeature>() &&
        !object.isType<GeoJsonFeatureCollection>() &&
        !object.isType<GeoJsonGeometryCollection>()) {
      this->_result.primitives.emplace_back(
          ResolvedPrimitive{&object, this->getStyleIndex(style)});
    }

    std::visit(ChildVisitor{*this, style}, object.value);
  }

private:
  // Every style passed in here lives in the document or is the default style,
  // so its address identifies it while the document is being walked. That lets
  // us store one copy per distinct style rather than one per object.
  uint32_t getStyleIndex(const VectorStyle& style) {
    auto [it, inserted] = this->_indices.try_emplace(
        &style,
        static_cast<uint32_t>(this->_result.styles.size()));
    if (inserted) {
      this->_result.styles.emplace_back(style);
    }
    return it->second;
  }

  struct ChildVisitor {
    StyleResolver& resolver;
    const VectorStyle& style;

    void operator()(const GeoJsonFeature& feature) {
      if (feature.geometry) {
        const std::optional<VectorStyle>& geometryStyle =
            feature.geometry->getStyle();
        const std::optional<VectorStyle>& featureStyle =
            geometryStyle ? geometryStyle : feature.style;
        resolver.addObject(
            *feature.geometry,
            featureStyle ? *featureStyle : style);
      }
    }

    void operator()(const GeoJsonFeatureCollection& collection) {
      for (const GeoJsonObject& feature : collection.features) {
        const GeoJsonFeature* pFeature = feature.getIf<GeoJsonFeature>();
        if (pFeature && pFeature->geometry) {
          const std::optional<VectorStyle>& geometryStyle =
              pFeature->geometry->getStyle();
          const std::optional<VectorStyle>& featureStyle =
              geometryStyle ? geometryStyle : pFeature->style;
          const std::optional<VectorStyle>& collectionStyle =
              featureStyle ? featureStyle : collection.style;
          resolver.addObject(
              *pFeature->geometry,
              collectionStyle ? *collectionStyle : style);
        }
      }
    }

    void operator()(const GeoJsonGeometryCollection& collection) {
      for (const GeoJsonObject& geometry : collection.geometries) {
        const std::optional<VectorStyle>& childStyle = geometry.getStyle();
        const std::optional<VectorStyle>& useStyle =
            childStyle ? childStyle : collection.style;
        resolver.addObject(geometry, useStyle ? *useStyle : style);
      }
    }

    void operator()(const auto& /*catchAll*/) {}
  };

  ResolvedDocument& _result;
  std::unordered_map<const VectorStyle*, uint32_t> _indices;
};

/**
 * @brief Resolves the style of every geometry object in the document. Must be
 * called on the main thread.
 */
ResolvedDocument resolveDocument(
    const GeoJsonDocument& document,
    const VectorStyle& defaultStyle) {
  ResolvedDocument result;
  StyleResolver resolver(result);
  const std::optional<VectorStyle>& rootObjectStyle =
      document.rootObject.getStyle();
  resolver.addObject(
      document.rootObject,
      rootObjectStyle ? *rootObjectStyle : defaultStyle);
  return result;
}

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

  double maxLineWidthPixels = 0.0;
  for (std::vector<uint32_t>::iterator it = begin; it != end; ++it) {
    maxLineWidthPixels =
        std::max(tree.data[*it].maxLineWidthPixels, maxLineWidthPixels);
  }

  tree.nodes[resultId].maxLineWidthPixels = maxLineWidthPixels;

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

/**
 * @brief Builds the quadtree for a resolved document. This may be called on a
 * worker thread, as long as the document's geometry isn't modified.
 */
void buildQuadtree(
    Quadtree& tree,
    ResolvedDocument&& resolved,
    const Ellipsoid& ellipsoid) {
  tree.styles = std::move(resolved.styles);

  BoundingRegionBuilder builder;
  tree.data.reserve(resolved.primitives.size());
  for (const ResolvedPrimitive& primitive : resolved.primitives) {
    std::optional<GlobeRectangle> rect;
    double maxLineWidthPixels = 0.0;
    std::visit(
        RectangleAndLineWidthFromObjectVisitor{
            rect,
            maxLineWidthPixels,
            tree.styles[primitive.styleIndex],
            ellipsoid},
        primitive.pObject->value);
    // Geometry without any coordinates has nothing to render.
    if (rect) {
      builder.expandToIncludeGlobeRectangle(*rect);
      tree.data.emplace_back(QuadtreeGeometryData{
          primitive.pObject,
          primitive.styleIndex,
          *rect,
          maxLineWidthPixels});
    }
  }

  tree.rectangle = builder.toGlobeRectangle();

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
  tree.dataNodeIndicesBegin.emplace_back((uint32_t)tree.dataIndices.size());
}

void rasterizeQuadtreeNode(
    const Quadtree& tree,
    uint32_t nodeId,
    const GlobeRectangle& rectangle,
    VectorRasterizer& rasterizer,
    std::vector<bool>& primitivesRendered,
    const glm::dvec2& halfPixelSize) {
  const QuadtreeNode& node = tree.nodes[nodeId];
  const GlobeRectangle scaledNodeRectangle =
      node.getRectangleScaledWithPixelSize(halfPixelSize);
  // If this node has no children, or if it is entirely within the target
  // rectangle, let's rasterize this node's contents and not any children.
  if (!node.anyChildren() ||
      (rectangle.contains(scaledNodeRectangle.getSouthwest()) &&
       rectangle.contains(scaledNodeRectangle.getNortheast()))) {
    for (uint32_t i = tree.dataNodeIndicesBegin[nodeId];
         i < tree.dataNodeIndicesBegin[nodeId + 1];
         i++) {
      const uint32_t dataIdx = tree.dataIndices[i];
      if (primitivesRendered[dataIdx]) {
        continue;
      }
      primitivesRendered[dataIdx] = true;
      const QuadtreeGeometryData& data = tree.data[dataIdx];
      rasterizer.drawGeoJsonObject(*data.pObject, tree.styles[data.styleIndex]);
    }
  } else {
    for (size_t i = 0; i < 2; i++) {
      for (size_t j = 0; j < 2; j++) {
        if (tree.nodes[node.children[i][j]]
                .getRectangleScaledWithPixelSize(halfPixelSize)
                .computeIntersection(rectangle)
                .has_value()) {
          rasterizeQuadtreeNode(
              tree,
              node.children[i][j],
              rectangle,
              rasterizer,
              primitivesRendered,
              halfPixelSize);
        }
      }
    }
  }
}

void rasterizeVectorData(
    LoadedRasterOverlayImage& result,
    const GlobeRectangle& rectangle,
    const Quadtree& tree,
    const Ellipsoid& ellipsoid,
    const glm::dvec2& halfPixelSize) {
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
        primitivesRendered,
        halfPixelSize);
    rasterizer.finalize();
  }
}
} // namespace

struct GeoJsonDocumentRasterOverlay::SharedState {
  /**
   * @brief The document passed to the constructor.
   *
   * This is shared, so that every tile provider created for the overlay (for
   * example when it's added to more than one tileset, or removed and added
   * again) can use it.
   */
  CesiumAsync::SharedFuture<std::shared_ptr<GeoJsonDocument>> documentFuture;

  /**
   * @brief The document set by `setDocument`, which replaces the one from
   * `documentFuture`.
   */
  std::shared_ptr<const GeoJsonDocument> pDocument;

  VectorStyle defaultStyle;

  /**
   * @brief Incremented every time the document, its styles, or the default
   * style change. Each tile provider rebuilds its quadtree when this no longer
   * matches the version it was built from.
   */
  uint64_t version = 0;
};

namespace {

/**
 * @brief Finds the regions where a rebuilt quadtree renders differently than
 * the old one, and invalidates them.
 */
void invalidateChangedRegions(
    RasterOverlayTileProvider& provider,
    const Quadtree& oldTree,
    const Quadtree& newTree,
    bool sameDocument) {
  bool sameGeometry =
      sameDocument && oldTree.data.size() == newTree.data.size();
  for (size_t i = 0; sameGeometry && i < oldTree.data.size(); ++i) {
    sameGeometry = oldTree.data[i].pObject == newTree.data[i].pObject;
  }

  if (sameGeometry) {
    // Only styles changed. Invalidate each geometry whose style changed,
    // covering both its old and new extent, since a style can change the area
    // a geometry covers (line widths in meters) or the number of pixels it
    // spills over (line widths and point sizes in pixels).
    for (size_t i = 0; i < oldTree.data.size(); ++i) {
      const QuadtreeGeometryData& oldData = oldTree.data[i];
      const QuadtreeGeometryData& newData = newTree.data[i];
      if (oldTree.styles[oldData.styleIndex] ==
          newTree.styles[newData.styleIndex]) {
        continue;
      }

      provider.invalidate(
          oldData.rectangle.computeUnion(newData.rectangle),
          std::max(oldData.maxLineWidthPixels, newData.maxLineWidthPixels));
    }
    return;
  }

  // The geometry changed, so we can't match up the old and new objects.
  // Invalidate everything either tree rendered.
  for (const Quadtree* pTree : {&oldTree, &newTree}) {
    if (pTree->nodes.empty()) {
      continue;
    }
    provider.invalidate(
        pTree->rectangle,
        pTree->nodes[pTree->rootId].maxLineWidthPixels);
  }
}

} // namespace

class CESIUMVECTOROVERLAYS_API GeoJsonDocumentRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::shared_ptr<GeoJsonDocumentRasterOverlay::SharedState> _pState;
  std::shared_ptr<const GeoJsonDocument> _pDocument;
  std::shared_ptr<Quadtree> _pTree;
  Ellipsoid _ellipsoid;
  uint32_t _mipLevels;
  // The value of `SharedState::version` that `_pTree` reflects.
  uint64_t _builtVersion;
  bool _rebuildInProgress;

public:
  GeoJsonDocumentRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const GeoJsonDocumentRasterOverlayOptions& geoJsonOptions,
      const std::shared_ptr<GeoJsonDocumentRasterOverlay::SharedState>& pState,
      std::shared_ptr<const CesiumVectorData::GeoJsonDocument>&& pDocument)
      : RasterOverlayTileProvider(
            pCreator,
            parameters,
            GeographicProjection(geoJsonOptions.ellipsoid),
            projectRectangleSimple(
                GeographicProjection(geoJsonOptions.ellipsoid),
                GlobeRectangle::MAXIMUM)),
        _pState(pState),
        _pDocument(std::move(pDocument)),
        _pTree(),
        _ellipsoid(geoJsonOptions.ellipsoid),
        _mipLevels(geoJsonOptions.mipLevels),
        _builtVersion(pState->version),
        _rebuildInProgress(false) {
    CESIUM_ASSERT(this->_pDocument);
    this->_pTree = std::make_shared<Quadtree>();
    buildQuadtree(
        *this->_pTree,
        resolveDocument(*this->_pDocument, this->_pState->defaultStyle),
        this->_ellipsoid);
  }

  virtual void tick() override {
    if (this->_rebuildInProgress ||
        this->_builtVersion == this->_pState->version) {
      return;
    }

    // Resolve and copy the styles here, on the main thread, because the caller
    // may change the document's styles in place on the main thread. The
    // geometry isn't allowed to change, so the rest of the work can happen on
    // a worker thread.
    const uint64_t version = this->_pState->version;
    std::shared_ptr<const GeoJsonDocument> pDocument =
        this->_pState->pDocument ? this->_pState->pDocument : this->_pDocument;
    ResolvedDocument resolved =
        resolveDocument(*pDocument, this->_pState->defaultStyle);

    this->_rebuildInProgress = true;

    IntrusivePointer<GeoJsonDocumentRasterOverlayTileProvider> thiz = this;
    this->getAsyncSystem()
        .runInWorkerThread([pDocument,
                            resolved = std::move(resolved),
                            ellipsoid = this->_ellipsoid]() mutable {
          // `pDocument` is captured to keep the geometry alive while the tree
          // is built.
          std::shared_ptr<Quadtree> pTree = std::make_shared<Quadtree>();
          buildQuadtree(*pTree, std::move(resolved), ellipsoid);
          return pTree;
        })
        .thenInMainThread([thiz, version, pDocument](
                              std::shared_ptr<Quadtree>&& pNewTree) mutable {
          thiz->finishRebuild(
              version,
              std::move(pDocument),
              std::move(pNewTree));
        })
        .catchInMainThread([thiz, version](const std::exception& e) {
          SPDLOG_LOGGER_ERROR(
              thiz->getLogger(),
              "Failed to update GeoJSON raster overlay: {}",
              e.what());
          // Don't retry the same version over and over.
          thiz->_builtVersion = version;
          thiz->_rebuildInProgress = false;
        });
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    // Choose the texture size according to the geometry screen size and raster
    // SSE, but no larger than the maximum texture size.
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::ivec2 textureSize = glm::max(
        glm::min(
            glm::ivec2(
                overlayTile.getTargetScreenPixels() /
                options.maximumScreenSpaceError),
            glm::ivec2(options.maximumTextureSize)),
        glm::ivec2(1));

    return this->getAsyncSystem().runInWorkerThread(
        [pTree = this->_pTree,
         pDocument = this->_pDocument,
         ellipsoid = this->_ellipsoid,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle(),
         textureSize,
         mipLevels = this->_mipLevels]() -> LoadedRasterOverlayImage {
          const Quadtree& tree = *pTree;
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage result;
          result.rectangle = rectangle;

          const glm::dvec2 halfPixelSize(
              tileRectangle.computeWidth() / (double)textureSize.x / 2.0,
              tileRectangle.computeHeight() / (double)textureSize.y / 2.0);

          // A document with nothing to render has no quadtree nodes at all.
          if (tree.nodes.empty() ||
              !tree.nodes[tree.rootId]
                   .getRectangleScaledWithPixelSize(halfPixelSize)
                   .computeIntersection(tileRectangle)
                   .has_value()) {
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
                    CesiumImage::ImageAssetMipPosition{
                        totalSize,
                        (size_t)(width * height * result.pImage->channels *
                                 result.pImage->bytesPerChannel)});
                totalSize += result.pImage->mipPositions[i].byteSize;
              }
              result.pImage->pixelData.resize(totalSize, std::byte{0});
            }
            rasterizeVectorData(
                result,
                tileRectangle,
                tree,
                ellipsoid,
                halfPixelSize);
          }

          return result;
        });
  }

private:
  void finishRebuild(
      uint64_t version,
      std::shared_ptr<const GeoJsonDocument>&& pDocument,
      std::shared_ptr<Quadtree>&& pNewTree) {
    // Tiles that are already loading keep their own references to the old
    // tree and document, so it's safe to replace them here.
    const std::shared_ptr<Quadtree> pOldTree = std::move(this->_pTree);
    const bool sameDocument = pDocument == this->_pDocument;

    this->_pTree = std::move(pNewTree);
    this->_pDocument = std::move(pDocument);
    this->_builtVersion = version;
    this->_rebuildInProgress = false;

    // Tiles loaded from now on use the new tree; invalidate the ones that were
    // loaded from the old one and look different now.
    invalidateChangedRegions(*this, *pOldTree, *this->_pTree, sameDocument);
  }
};

GeoJsonDocumentRasterOverlay::GeoJsonDocumentRasterOverlay(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& name,
    const std::shared_ptr<GeoJsonDocument>& document,
    const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _pState(std::make_shared<SharedState>(SharedState{
          asyncSystem
              .createResolvedFuture(std::shared_ptr<GeoJsonDocument>(document))
              .share(),
          nullptr,
          vectorOverlayOptions.defaultStyle})),
      _options(vectorOverlayOptions) {}

GeoJsonDocumentRasterOverlay::GeoJsonDocumentRasterOverlay(
    const std::string& name,
    CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>&&
        documentFuture,
    const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _pState(std::make_shared<SharedState>(SharedState{
          std::move(documentFuture).share(),
          nullptr,
          vectorOverlayOptions.defaultStyle})),
      _options(vectorOverlayOptions) {}

GeoJsonDocumentRasterOverlay::~GeoJsonDocumentRasterOverlay() = default;

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
GeoJsonDocumentRasterOverlay::createTileProvider(
    const CreateRasterOverlayTileProviderParameters& parameters) const {

  IntrusivePointer<const GeoJsonDocumentRasterOverlay> thiz = this;

  return this->_pState->documentFuture.thenInMainThread(
      [thiz, parameters, pState = this->_pState](
          const std::shared_ptr<GeoJsonDocument>& pLoadedDocument)
          -> CreateTileProviderResult {
        std::shared_ptr<const GeoJsonDocument> pDocument =
            pState->pDocument ? pState->pDocument : pLoadedDocument;
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
                pState,
                std::move(pDocument)));
      });
}

void GeoJsonDocumentRasterOverlay::setDocument(
    const std::shared_ptr<const GeoJsonDocument>& pDocument) {
  CESIUM_ASSERT(pDocument);
  if (!pDocument) {
    return;
  }

  this->_pState->pDocument = pDocument;
  ++this->_pState->version;
}

void GeoJsonDocumentRasterOverlay::refreshStyles() { ++this->_pState->version; }

const VectorStyle&
GeoJsonDocumentRasterOverlay::getDefaultStyle() const noexcept {
  return this->_pState->defaultStyle;
}

void GeoJsonDocumentRasterOverlay::setDefaultStyle(const VectorStyle& style) {
  this->_pState->defaultStyle = style;
  this->_options.defaultStyle = style;
  ++this->_pState->version;
}

} // namespace CesiumVectorOverlays
