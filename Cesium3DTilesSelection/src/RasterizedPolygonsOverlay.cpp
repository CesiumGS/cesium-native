
#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "TileUtilities.h"
#include <memory>
#include <string>

using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {
namespace {
void rasterizePolygons(
    CesiumGltf::ImageCesium& image,
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CartographicPolygon>& cartographicPolygons) {

  // create a 1x1 mask if the rectangle is completely inside a polygon
  if (Cesium3DTilesSelection::Impl::withinPolygons(
          rectangle,
          cartographicPolygons)) {
    image.width = 1;
    image.height = 1;
    image.channels = 1;
    image.bytesPerChannel = 1;
    image.pixelData.resize(1);

    image.pixelData[0] = static_cast<std::byte>(0xff);

    return;
  }

  bool completelyOutsidePolygons = true;
  for (const CartographicPolygon& selection : cartographicPolygons) {
    const std::optional<CesiumGeospatial::GlobeRectangle>& boundingRectangle =
        selection.getBoundingRectangle();

    if (boundingRectangle &&
        rectangle.computeIntersection(*boundingRectangle)) {
      completelyOutsidePolygons = false;
      break;
    }
  }

  // create a 1x1 mask if the rectangle is completely outside all polygons
  if (completelyOutsidePolygons) {
    image.width = 1;
    image.height = 1;
    image.channels = 1;
    image.bytesPerChannel = 1;
    image.pixelData.resize(1);

    return;
  }

  const double rectangleWidth = rectangle.computeWidth();
  const double rectangleHeight = rectangle.computeHeight();

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
  for (const CartographicPolygon& polygon : cartographicPolygons) {
    const std::vector<glm::dvec2>& vertices = polygon.getVertices();
    const std::vector<uint32_t>& indices = polygon.getIndices();
    for (size_t triangle = 0; triangle < indices.size() / 3; ++triangle) {
      const glm::dvec2& a = vertices[indices[3 * triangle]];
      const glm::dvec2& b = vertices[indices[3 * triangle + 1]];
      const glm::dvec2& c = vertices[indices[3 * triangle + 2]];

      // TODO: deal with the corner cases here
      const double minX = glm::min(a.x, glm::min(b.x, c.x));
      const double minY = glm::min(a.y, glm::min(b.y, c.y));
      const double maxX = glm::max(a.x, glm::max(b.x, c.x));
      const double maxY = glm::max(a.y, glm::max(b.y, c.y));

      const CesiumGeospatial::GlobeRectangle triangleBounds(minX, minY, maxX, maxY);

      // skip this triangle if it is entirely outside the tile bounds
      if (!rectangle.computeIntersection(triangleBounds)) {
        continue;
      }

      const glm::dvec2 ab = b - a;
      const glm::dvec2 ab_perp(-ab.y, ab.x);
      const glm::dvec2 bc = c - b;
      const glm::dvec2 bc_perp(-bc.y, bc.x);
      const glm::dvec2 ca = a - c;
      const glm::dvec2 ca_perp(-ca.y, ca.x);

      for (size_t j = 0; j < 256; ++j) {
        const double pixelY = rectangle.getSouth() +
                        rectangleHeight * (1.0 - (double(j) + 0.5) / 256.0);
        for (size_t i = 0; i < 256; ++i) {
          const double pixelX =
              rectangle.getWest() + rectangleWidth * (double(i) + 0.5) / 256.0;
          const glm::dvec2 v(pixelX, pixelY);

          const glm::dvec2 av = v - a;
          const glm::dvec2 cv = v - c;

          const double v_proj_ab_perp = glm::dot(av, ab_perp);
          const double v_proj_bc_perp = glm::dot(cv, bc_perp);
          const double v_proj_ca_perp = glm::dot(cv, ca_perp);

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

class CESIUM3DTILESSELECTION_API RasterizedPolygonsTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::vector<CartographicPolygon> _polygons;

public:
  RasterizedPolygonsTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const std::vector<CartographicPolygon>& polygons)
      : RasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            projection),
        _polygons(polygons) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) override {
    return this->getAsyncSystem().runInWorkerThread(
        [&polygons = this->_polygons,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle()]() -> LoadedRasterOverlayImage {
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage resultImage;
          resultImage.rectangle = rectangle;
          CesiumGltf::ImageCesium image;
          rasterizePolygons(image, tileRectangle, polygons);
          resultImage.image = std::move(image);
          return resultImage;
        });
  }
};

RasterizedPolygonsOverlay::RasterizedPolygonsOverlay(
    const std::string& name,
    const std::vector<CartographicPolygon>& polygons,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::Projection& projection)
    : RasterOverlay(name),
      _polygons(polygons),
      _ellipsoid(ellipsoid),
      _projection(projection) {}

RasterizedPolygonsOverlay::~RasterizedPolygonsOverlay() {}

CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
RasterizedPolygonsOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& /*pCreditSystem*/,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {

  pOwner = pOwner ? pOwner : this;

  return asyncSystem.createResolvedFuture(
      (std::unique_ptr<RasterOverlayTileProvider>)
          std::make_unique<RasterizedPolygonsTileProvider>(
              *this,
              asyncSystem,
              pAssetAccessor,
              pPrepareRendererResources,
              pLogger,
              this->_projection,
              this->_polygons));
}

} // namespace Cesium3DTilesSelection
