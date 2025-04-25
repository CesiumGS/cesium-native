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
#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
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
    const CesiumGeospatial::Ellipsoid& ellipsoid,
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

  // create source image
  loaded.moreDetailAvailable = true;
  image.width = int32_t(glm::round(textureSize.x));
  image.height = int32_t(glm::round(textureSize.y));
  image.channels = 4;
  image.bytesPerChannel = 1;
  image.pixelData.resize(size_t(image.width * image.height * 4), outsideColor);

  CesiumVectorData::VectorRasterizer rasterizer(
      rectangle,
      loaded.pImage,
      ellipsoid);
  rasterizer.clear(CesiumVectorData::Color{
      outsideColor,
      std::byte{0x00},
      std::byte{0x00},
      std::byte{0xff}});

  CesiumVectorData::VectorRasterizerStyle insideStyle{CesiumVectorData::Color{
      insideColor,
      std::byte{0x00},
      std::byte{0x00},
      std::byte{0xff}}};
  for (const CartographicPolygon& selection : cartographicPolygons) {
    rasterizer.drawPolygon(selection, insideStyle);
  }

  rasterizer.finalize();
  // Convert RGBA32 -> R8
  image.convertToChannels(1);
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
              getProjectionEllipsoid(projection),
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
