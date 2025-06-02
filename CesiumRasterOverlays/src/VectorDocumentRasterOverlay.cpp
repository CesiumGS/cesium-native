#include "CesiumAsync/Future.h"
#include "CesiumVectorData/VectorStyle.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/VectorDocumentRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/joinToString.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumRasterOverlays {

namespace {
struct QuadtreeGeometryData {
  const GeoJsonObject* pObject;
  const VectorStyle* pStyle;
  GlobeRectangle rectangle;
};

struct QuadtreeNode {
  GlobeRectangle rectangle;
  uint32_t children[2][2] = {{0, 0}, {0, 0}};

  QuadtreeNode(const GlobeRectangle& rectangle_) : rectangle(rectangle_) {}
  bool anyChildren() const {
    return children[0][0] != 0 && children[0][1] != 0 && children[1][0] != 0 &&
           children[1][1] != 0;
  }
};

struct Quadtree {
  GlobeRectangle rectangle = GlobeRectangle::EMPTY;
  uint32_t rootId = 0;
  std::vector<QuadtreeNode> nodes;
  std::vector<QuadtreeGeometryData> data;
  std::vector<uint32_t> dataIndices;
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
    const GeoJsonObject* geoJsonObject,
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
  documentRegionBuilder.expandToIncludePosition(rect.getSouthwest());
  documentRegionBuilder.expandToIncludePosition(rect.getNortheast());
  data.emplace_back(
      QuadtreeGeometryData{geoJsonObject, &style, std::move(rect)});

  std::visit(
      GeoJsonChildVisitor{data, documentRegionBuilder, style},
      geoJsonObject->value);
}

const uint32_t DEPTH_LIMIT = 8;
uint32_t buildQuadtreeNode(
    Quadtree& tree,
    const GlobeRectangle& rectangle,
    std::vector<uint32_t>::iterator begin,
    std::vector<uint32_t>::iterator end,
    uint32_t depth) {
  if (begin == end) {
    return 0;
  }

  uint32_t resultId = (uint32_t)tree.nodes.size();
  tree.nodes.emplace_back(rectangle);

  uint32_t indicesBegin = (uint32_t)tree.dataIndices.size();
  tree.dataIndices.insert(tree.dataIndices.end(), begin, end);

  tree.dataNodeIndicesBegin.emplace_back(indicesBegin);

  if (begin + 1 == end || depth >= DEPTH_LIMIT ||
      std::equal(begin + 1, end, begin)) {
    return resultId;
  }

  Cartographic southWest = rectangle.getSouthwest();
  Cartographic northEast = rectangle.getNortheast();
  Cartographic center = rectangle.computeCenter();

  const GlobeRectangle southWestRect = GlobeRectangle(
      southWest.longitude,
      southWest.latitude,
      center.longitude,
      center.latitude);
  const GlobeRectangle southEastRect = GlobeRectangle(
      center.longitude,
      southWest.latitude,
      northEast.longitude,
      center.latitude);
  const GlobeRectangle northWestRect = GlobeRectangle(
      southWest.longitude,
      center.latitude,
      center.longitude,
      northEast.latitude);
  const GlobeRectangle northEastRect = GlobeRectangle(
      center.longitude,
      center.latitude,
      northEast.longitude,
      northEast.latitude);

  tree.nodes[resultId].children[0][0] = buildQuadtreeNode(
      tree,
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
      depth + 1);
  tree.nodes[resultId].children[0][1] = buildQuadtreeNode(
      tree,
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
      depth + 1);
  tree.nodes[resultId].children[1][0] = buildQuadtreeNode(
      tree,
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
      depth + 1);
  tree.nodes[resultId].children[1][1] = buildQuadtreeNode(
      tree,
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
      depth + 1);

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

  tree.rootId = buildQuadtreeNode(
      tree,
      tree.rectangle,
      dataIndices.begin(),
      dataIndices.end(),
      0);
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
      rasterizer.drawGeoJsonObject(data.pObject, *data.pStyle);
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
  for (size_t i = 0;
       i < std::max(result.pImage->mipPositions.size(), (size_t)1);
       i++) {
    VectorRasterizer rasterizer(
        rectangle,
        result.pImage,
        (uint32_t)i,
        ellipsoid);
    // Keeps track of primitives that have already been rendered to avoid
    // re-drawing the same primitives that appear in multiple quadtree nodes.
    std::vector<bool> primitivesRendered(tree.data.size(), false);
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

class CESIUMRASTEROVERLAYS_API VectorDocumentRasterOverlayTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::shared_ptr<GeoJsonDocument> _document;
  VectorStyle _defaultStyle;
  Quadtree _tree;
  Ellipsoid _ellipsoid;
  uint32_t _mipLevels;
  std::optional<VectorDocumentRasterOverlayStyleCallback> _styleCallback;

public:
  VectorDocumentRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const VectorDocumentRasterOverlayOptions& options,
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document)
      : RasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            options.projection,
            projectRectangleSimple(
                options.projection,
                CesiumGeospatial::GlobeRectangle::MAXIMUM)),
        _document(document),
        _defaultStyle(options.defaultStyle),
        _tree(),
        _ellipsoid(options.ellipsoid),
        _mipLevels(options.mipLevels),
        _styleCallback(options.styleCallback) {
    // Compute styles before building the quadtree so we can cache the computed
    // styles in QuadtreePrimitiveData
    this->recomputeStyles();
    this->_tree = buildQuadtree(this->_document, this->_defaultStyle);
  }

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
         &tree = this->_tree,
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

  void
  setStyleCallback(VectorDocumentRasterOverlayStyleCallback&& newCallback) {
    this->_styleCallback = std::move(newCallback);
  }

  void recomputeStyles() {
    if (!this->_styleCallback) {
      return;
    }

    this->recomputeStyles(&this->_document->rootObject);
  }

private:
  void recomputeStyles(GeoJsonObject* pObject) {
    struct SetStyleVisitor {
      const std::optional<VectorStyle>& style;
      void operator()(GeoJsonPoint& o) { o.style = style; }
      void operator()(GeoJsonMultiPoint& o) { o.style = style; }
      void operator()(GeoJsonLineString& o) { o.style = style; }
      void operator()(GeoJsonMultiLineString& o) { o.style = style; }
      void operator()(GeoJsonPolygon& o) { o.style = style; }
      void operator()(GeoJsonMultiPolygon& o) { o.style = style; }
      void operator()(GeoJsonFeature& o) { o.style = style; }
      void operator()(GeoJsonFeatureCollection& o) { o.style = style; }
      void operator()(GeoJsonGeometryCollection& o) { o.style = style; }
    };

    if (this->_styleCallback) {
      const std::optional<VectorStyle>& style =
          (*this->_styleCallback)(this->_document, pObject);
      std::visit(SetStyleVisitor{style}, pObject->value);
    }

    struct RecomputeChildStylesVisitor {
      VectorDocumentRasterOverlayTileProvider* pThis;
      void operator()(GeoJsonFeature& feature) {
        if (feature.geometry) {
          pThis->recomputeStyles(feature.geometry.get());
        }
      }
      void operator()(GeoJsonFeatureCollection& features) {
        for (GeoJsonObject& feature : features.features) {
          const GeoJsonFeature* pFeature = feature.getIf<GeoJsonFeature>();
          if (pFeature && pFeature->geometry) {
            pThis->recomputeStyles(pFeature->geometry.get());
          }
        }
      }
      void operator()(GeoJsonGeometryCollection& collection) {
        for (GeoJsonObject& geometry : collection.geometries) {
          pThis->recomputeStyles(&geometry);
        }
      }
      void operator()(GeoJsonPoint& /*lhs*/) {}
      void operator()(GeoJsonMultiPoint& /*lhs*/) {}
      void operator()(GeoJsonLineString& /*lhs*/) {}
      void operator()(GeoJsonMultiLineString& /*lhs*/) {}
      void operator()(GeoJsonPolygon& /*lhs*/) {}
      void operator()(GeoJsonMultiPolygon& /*lhs*/) {}
    };

    std::visit(RecomputeChildStylesVisitor{this}, pObject->value);
  }
};

VectorDocumentRasterOverlay::VectorDocumentRasterOverlay(
    const std::string& name,
    const VectorDocumentRasterOverlaySource& source,
    const VectorDocumentRasterOverlayOptions& vectorOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _source(source),
      _options(vectorOptions) {}

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

  struct DocumentSourceVisitor {
    CesiumAsync::AsyncSystem asyncSystem;
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;
    CesiumAsync::Future<Result<std::shared_ptr<GeoJsonDocument>>>
    operator()(const std::shared_ptr<GeoJsonDocument>& document) {
      return asyncSystem
          .createResolvedFuture<Result<std::shared_ptr<GeoJsonDocument>>>(
              Result(document));
    }
    CesiumAsync::Future<Result<std::shared_ptr<GeoJsonDocument>>>
    operator()(const IonVectorDocumentRasterOverlaySource& ion) {
      return GeoJsonDocument::fromCesiumIonAsset(
                 asyncSystem,
                 pAssetAccessor,
                 ion.ionAssetID,
                 ion.ionAccessToken,
                 ion.ionAssetEndpointUrl)
          .thenImmediately([](Result<GeoJsonDocument>&& result) {
            if (!result.value) {
              return Result<std::shared_ptr<GeoJsonDocument>>(result.errors);
            }

            return Result<std::shared_ptr<GeoJsonDocument>>(
                std::make_shared<GeoJsonDocument>(std::move(*result.value)));
          });
    }
  };

  return std::visit(
             DocumentSourceVisitor{asyncSystem, pAssetAccessor},
             this->_source)
      .thenImmediately(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           pPrepareRendererResources,
           pLogger,
           options = this->_options](
              Result<std::shared_ptr<GeoJsonDocument>>&& result)
              -> CreateTileProviderResult {
            if (!result.value) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::CesiumIon,
                  nullptr,
                  fmt::format(
                      "Errors while loading GeoJSON from Cesium ion: {}",
                      CesiumUtility::joinToString(
                          result.errors.errors,
                          ", "))});
            }

            return IntrusivePointer<RasterOverlayTileProvider>(
                new VectorDocumentRasterOverlayTileProvider(
                    pOwner,
                    asyncSystem,
                    pAssetAccessor,
                    pPrepareRendererResources,
                    pLogger,
                    options,
                    *result.value));
          });
}

} // namespace CesiumRasterOverlays
