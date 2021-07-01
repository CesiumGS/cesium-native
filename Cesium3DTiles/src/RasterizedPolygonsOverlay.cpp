
#include "Cesium3DTiles/RasterizedPolygonsOverlay.h"
#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <memory>
#include <string>

#include "TileUtilities.h"

namespace Cesium3DTiles {
namespace {
void rasterizePolygons(
    CesiumGltf::ImageCesium& image,
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::string& textureTargetName,
    const std::vector<CartographicSelection>& cartographicSelections) {

  if (Cesium3DTiles::Impl::withinPolygons(rectangle, cartographicSelections)) {
    image.width = 1;
    image.height = 1;
    image.channels = 1;
    image.bytesPerChannel = 1;
    image.pixelData.resize(1);

    image.pixelData[0] = static_cast<std::byte>(0xff);

    return;
  }

  // TODO: for tiles completely outside polygons also include 1 pixel texture

  double rectangleWidth = rectangle.computeWidth();
  double rectangleHeight = rectangle.computeHeight();

  // create source image
  image.width = 256;
  image.height = 256;
  image.channels = 1;
  image.bytesPerChannel = 1;
  image.pixelData.resize(65536);

  // TODO: this is naive approach, use line-triangle
  // intersections to rasterize one row at a time
  // NOTE: also completely ignores antimeridian (really these
  // calculations should be normalized to the first vertex)
  for (const CartographicSelection& selection : cartographicSelections) {

    if (selection.getTargetTextureName() != textureTargetName) {
      continue;
    }

    const std::vector<glm::dvec2>& vertices = selection.getVertices();
    const std::vector<uint32_t>& indices = selection.getIndices();
    for (size_t triangle = 0; triangle < indices.size() / 3; ++triangle) {
      const glm::dvec2& a = vertices[indices[3 * triangle]];
      const glm::dvec2& b = vertices[indices[3 * triangle + 1]];
      const glm::dvec2& c = vertices[indices[3 * triangle + 2]];

      // TODO: deal with the corner cases here
      double minX = glm::min(a.x, glm::min(b.x, c.x));
      double minY = glm::min(a.y, glm::min(b.y, c.y));
      double maxX = glm::max(a.x, glm::max(b.x, c.x));
      double maxY = glm::max(a.y, glm::max(b.y, c.y));

      CesiumGeospatial::GlobeRectangle triangleBounds(minX, minY, maxX, maxY);

      // skip this triangle if it is entirely outside the tile bounds
      if (!rectangle.intersect(triangleBounds)) {
        continue;
      }

      glm::dvec2 ab = b - a;
      glm::dvec2 ab_perp(-ab.y, ab.x);
      glm::dvec2 bc = c - b;
      glm::dvec2 bc_perp(-bc.y, bc.x);
      glm::dvec2 ca = a - c;
      glm::dvec2 ca_perp(-ca.y, ca.x);

      for (size_t j = 0; j < 256; ++j) {
        double pixelY =
            rectangle.getSouth() + rectangleHeight * (double(j) + 0.5) / 256.0;
        for (size_t i = 0; i < 256; ++i) {
          double pixelX =
              rectangle.getWest() + rectangleWidth * (double(i) + 0.5) / 256.0;
          glm::dvec2 v(pixelX, pixelY);

          glm::dvec2 av = v - a;
          glm::dvec2 cv = v - c;

          double v_proj_ab_perp = glm::dot(av, ab_perp);
          double v_proj_bc_perp = glm::dot(cv, bc_perp);
          double v_proj_ca_perp = glm::dot(cv, ca_perp);

          // will determine in or out, irrespective of winding
          if ((v_proj_ab_perp >= 0.0 && v_proj_ca_perp >= 0.0 &&
               v_proj_bc_perp >= 0.0) ||
              (v_proj_ab_perp <= 0.0 && v_proj_ca_perp <= 0.0 &&
               v_proj_bc_perp <= 0.0)) {
            image.pixelData[256 * j + i] = static_cast<std::byte>(0xff);
          }
        }
      }
    }
  }
}
} // namespace

class CESIUM3DTILES_API RasterizedPolygonsTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::string _textureTargetName;
  std::vector<CartographicSelection> _polygons;

public:
  RasterizedPolygonsTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const std::string& textureTargetName,
      const std::vector<CartographicSelection>& polygons)
      : RasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            projection),
        _textureTargetName(textureTargetName),
        _polygons(polygons) {}

  virtual void mapRasterTilesToGeometryTile(
      const TileID& geometryTileId,
      const CesiumGeospatial::GlobeRectangle& geometryRectangle,
      double targetGeometricError,
      std::vector<Cesium3DTiles::RasterMappedTo3DTile>& outputRasterTiles,
      std::optional<size_t> outputIndex) override {

    for (const CartographicSelection& polygon : this->_polygons) {
      const std::optional<CesiumGeospatial::GlobeRectangle>& boundingRectangle =
          polygon.getBoundingRectangle();
      // TODO: should this intersection be tested on projected or unprojected
      // rectangle?
      if (boundingRectangle &&
          geometryRectangle.intersect(*boundingRectangle)) {
        this->mapRasterTilesToGeometryTile(
            geometryTileId,
            projectRectangleSimple(this->getProjection(), geometryRectangle),
            targetGeometricError,
            outputRasterTiles,
            outputIndex);
      }
    }
  }

  virtual void mapRasterTilesToGeometryTile(
      const TileID& geometryTileId,
      const CesiumGeometry::Rectangle& geometryRectangle,
      double /*targetGeometricError*/,
      std::vector<Cesium3DTiles::RasterMappedTo3DTile>& outputRasterTiles,
      std::optional<size_t> /*outputIndex*/) override {
    if (this->_pPlaceholder) {
      outputRasterTiles.push_back(
          RasterMappedTo3DTile(std::vector<RasterToCombine>({RasterToCombine(
              this->_pPlaceholder.get(),
              CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0))})));
      return;
    }

    CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile =
        this->getTile(geometryTileId, geometryRectangle);

    if (pTile->getState() != RasterOverlayTile::LoadState::Placeholder) {
      this->loadTileThrottled(*pTile);
    }

    outputRasterTiles.push_back(
        RasterMappedTo3DTile(std::vector<RasterToCombine>({RasterToCombine(
            pTile,
            CesiumGeometry::Rectangle(0.0, 0.0, 1.0, 1.0))})));
  }

  virtual bool
  hasMoreDetailsAvailable(const TileID& /*tileID*/) const override {
    return false; // always true or always false??
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const TileID& tileID) override {
    CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile =
        this->getTileWithoutCreating(tileID);

    return this->_asyncSystem.runInWorkerThread(
        [pTile = std::move(pTile),
         &textureTargetName = this->_textureTargetName,
         &polygons = this->_polygons,
         &projection = this->_projection]() -> LoadedRasterOverlayImage {
          CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(
                  projection,
                  pTile ? pTile->getImageryRectangle()
                        : CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0));

          LoadedRasterOverlayImage resultImage;
          CesiumGltf::ImageCesium image;
          rasterizePolygons(image, tileRectangle, textureTargetName, polygons);
          resultImage.image = std::move(image);
          return resultImage;
        });
  }
};

RasterizedPolygonsOverlay::RasterizedPolygonsOverlay(
    const std::string& textureTargetName,
    const std::vector<CartographicSelection>& polygons,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::Projection& projection)
    : RasterOverlay("CUSTOM_MASK_" + textureTargetName),
      _textureTargetName(textureTargetName),
      _polygons(polygons),
      _ellipsoid(ellipsoid),
      _projection(projection) {
  std::copy_if(
      polygons.begin(),
      polygons.end(),
      std::back_inserter(this->_clippingPolygons),
      [polygons](const CartographicSelection& polygon) {
        return polygon.isForCulling();
      });
}

RasterizedPolygonsOverlay::~RasterizedPolygonsOverlay() {}

CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
RasterizedPolygonsOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& /*pCreditSystem*/,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* /*pOwner*/) {
  return asyncSystem.createResolvedFuture(
      (std::unique_ptr<RasterOverlayTileProvider>)
          std::make_unique<RasterizedPolygonsTileProvider>(
              *this,
              asyncSystem,
              pAssetAccessor,
              pPrepareRendererResources,
              pLogger,
              this->_projection,
              this->_textureTargetName,
              this->_polygons));
}

} // namespace Cesium3DTiles