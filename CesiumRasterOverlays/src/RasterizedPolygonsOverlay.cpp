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
#include <CesiumRasterOverlays/RasterizedPolygonsOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

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

namespace CesiumRasterOverlays {
namespace {
void rasterizePolygons(
    LoadedRasterOverlayImage& loaded,
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const glm::dvec2& textureSize,
    const std::vector<CartographicPolygon>& cartographicPolygons,
    bool invertSelection) {

  CesiumGltf::ImageAsset& image = loaded.pImage.emplace();

  std::byte insideColor;
  std::byte outsideColor;

  if (invertSelection) {
    insideColor = static_cast<std::byte>(0);
    outsideColor = static_cast<std::byte>(0xff);
  } else {
    insideColor = static_cast<std::byte>(0xff);
    outsideColor = static_cast<std::byte>(0);
  }

  // create a 1x1 mask if the rectangle is completely inside a polygon
  if (CartographicPolygon::rectangleIsWithinPolygons(
          rectangle,
          cartographicPolygons)) {
    loaded.moreDetailAvailable = false;
    image.width = 1;
    image.height = 1;
    image.channels = 1;
    image.bytesPerChannel = 1;
    image.pixelData.resize(1, insideColor);
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
    loaded.moreDetailAvailable = false;
    image.width = 1;
    image.height = 1;
    image.channels = 1;
    image.bytesPerChannel = 1;
    image.pixelData.resize(1, outsideColor);
    return;
  }

  const double rectangleWidth = rectangle.computeWidth();
  const double rectangleHeight = rectangle.computeHeight();

  // create source image
  loaded.moreDetailAvailable = true;
  image.width = int32_t(glm::round(textureSize.x));
  image.height = int32_t(glm::round(textureSize.y));
  image.channels = 1;
  image.bytesPerChannel = 1;
  image.pixelData.resize(size_t(image.width * image.height), outsideColor);

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

      const CesiumGeospatial::GlobeRectangle triangleBounds(
          minX,
          minY,
          maxX,
          maxY);

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

      size_t width = size_t(image.width);
      size_t height = size_t(image.height);

      for (size_t j = 0; j < height; ++j) {
        const double pixelY =
            rectangle.getSouth() +
            rectangleHeight * (1.0 - (double(j) + 0.5) / double(height));
        for (size_t i = 0; i < width; ++i) {
          const double pixelX = rectangle.getWest() + rectangleWidth *
                                                          (double(i) + 0.5) /
                                                          double(width);
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
            image.pixelData[width * j + i] = insideColor;
          }
        }
      }
    }
  }
}
} // namespace

class CESIUMRASTEROVERLAYS_API RasterizedPolygonsTileProvider final
    : public RasterOverlayTileProvider {

private:
  std::vector<CartographicPolygon> _polygons;
  bool _invertSelection;

public:
  RasterizedPolygonsTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const std::vector<CartographicPolygon>& polygons,
      bool invertSelection)
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
        _polygons(polygons),
        _invertSelection(invertSelection) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) override {
    // Choose the texture size according to the geometry screen size and raster
    // SSE, but no larger than the maximum texture size.
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::dvec2 textureSize = glm::min(
        overlayTile.getTargetScreenPixels() / options.maximumScreenSpaceError,
        glm::dvec2(options.maximumTextureSize));

    return this->getAsyncSystem().runInWorkerThread(
        [&polygons = this->_polygons,
         invertSelection = this->_invertSelection,
         projection = this->getProjection(),
         rectangle = overlayTile.getRectangle(),
         textureSize]() -> LoadedRasterOverlayImage {
          const CesiumGeospatial::GlobeRectangle tileRectangle =
              CesiumGeospatial::unprojectRectangleSimple(projection, rectangle);

          LoadedRasterOverlayImage result;
          result.rectangle = rectangle;

          rasterizePolygons(
              result,
              tileRectangle,
              textureSize,
              polygons,
              invertSelection);

          return result;
        });
  }
};

RasterizedPolygonsOverlay::RasterizedPolygonsOverlay(
    const std::string& name,
    const std::vector<CartographicPolygon>& polygons,
    bool invertSelection,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const CesiumGeospatial::Projection& projection,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _polygons(polygons),
      _invertSelection(invertSelection),
      _ellipsoid(ellipsoid),
      _projection(projection) {}

RasterizedPolygonsOverlay::~RasterizedPolygonsOverlay() = default;

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
RasterizedPolygonsOverlay::createTileProvider(
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
          new RasterizedPolygonsTileProvider(
              pOwner,
              asyncSystem,
              pAssetAccessor,
              pPrepareRendererResources,
              pLogger,
              this->_projection,
              this->_polygons,
              this->_invertSelection)));
}

} // namespace CesiumRasterOverlays
