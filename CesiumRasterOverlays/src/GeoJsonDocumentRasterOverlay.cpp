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
const uint32_t DEPTH_LIMIT = 8;

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
    
    double newWest = this->rectangle.getWest() - halfX;
    if(newWest < -180.0) {
      newWest += 360.0;
    }
    double newEast = this->rectangle.getEast() + halfX;
    if(newEast > 180.0) {
      newEast -= 360.0;
    }

    return GlobeRectangle(
        newWest,
        this->rectangle.getSouth() - halfY,
        newEast,
        this->rectangle.getNorth() + halfY);
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
   * Indices into `tree.data` for every primitive that this node contains.
   */
  std::vector<uint32_t> data;
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
  BoundingRegionBuilder& builder;
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
        builder.expandToIncludePosition(Cartographic::fromDegrees(
            point.x - halfWidth,
            point.y - halfWidth));
        builder.expandToIncludePosition(Cartographic::fromDegrees(
            point.x + halfWidth,
            point.y + halfWidth));
      }
    } else {
      if (lineStyle && lineStyle->widthMode == LineWidthMode::Pixels) {
        this->maxLineWidthPixels =
            std::max(this->maxLineWidthPixels, lineStyle->width);
      }
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
  double maxLineWidthPixels;
  std::visit(
      RectangleAndLineWidthFromObjectVisitor{
          thisBuilder,
          maxLineWidthPixels,
          style,
          ellipsoid},
      geoJsonObject->value);
  GlobeRectangle rect = thisBuilder.toGlobeRectangle();
  // Points and MultiPoints, as well as Features, FeatureCollections, and
  // GeometryCollections will return a zero-size bounding box. For the first
  // two, this is because they are not rasterized by this overlay. For the rest,
  // though they may contain geometry, they do not themselves have anything to
  // render. We can save some effort by ignoring them all now.
  if (rect.computeWidth() != 0.0 || rect.computeHeight() != 0.0) {
    QuadtreeGeometryData primitive{geoJsonObject, &style, std::move(rect), maxLineWidthPixels};
    documentRegionBuilder.expandToIncludeGlobeRectangle(rect);
    data.emplace_back(primitive);
  }

  std::visit(
      GeoJsonChildVisitor{data, documentRegionBuilder, style, ellipsoid},
      geoJsonObject->value);
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
      rasterizer.drawGeoJsonObject(*data.pObject, *data.pStyle);
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
         ellipsoid = this->_ellipsoid,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle(),
         textureSize,
         mipLevels = this->_mipLevels]() -> LoadedRasterOverlayImage {
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage result;
          result.rectangle = rectangle;

          const glm::dvec2 halfPixelSize(
              tileRectangle.computeWidth() / (double)textureSize.x / 2.0,
              tileRectangle.computeHeight() / (double)textureSize.y / 2.0);

          if (!tree.nodes[tree.rootId]
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
                    CesiumGltf::ImageAssetMipPosition{
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
