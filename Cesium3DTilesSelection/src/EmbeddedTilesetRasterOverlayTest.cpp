#include "Cesium3DTilesSelection/TilesetSharedAssetSystem.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h"
#include "CesiumRasterOverlays/RasterOverlay.h"
#include "CesiumRasterOverlays/RasterOverlayTile.h"
#include "CesiumRasterOverlays/RasterOverlayTileProvider.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "TilesetContentManager.h"
#include "TilesetTileSelection.h"

#include <Cesium3DTilesSelection/EmbeddedTilesetRasterOverlayTest.h>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
class EmbeddedTilesetRasterOverlayTestTileProvider final
    : public RasterOverlayTileProvider {
private:
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
  CesiumUtility::IntrusivePointer<TilesetTileSelection> _pTileSelection;

public:
  EmbeddedTilesetRasterOverlayTestTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const std::string& url)
      : RasterOverlayTileProvider(
            pCreator,
            parameters,
            CesiumGeospatial::GeographicProjection(
                CesiumGeospatial::Ellipsoid::WGS84),
            projectRectangleSimple(
                CesiumGeospatial::GeographicProjection(
                    CesiumGeospatial::Ellipsoid::WGS84),
                CesiumGeospatial::GlobeRectangle::MAXIMUM)),
        _pTileSelection(new TilesetTileSelection()) {
    const TilesetExternals externals{
        parameters.externals.pAssetAccessor,
        nullptr,
        parameters.externals.asyncSystem,
        parameters.externals.pCreditSystem,
        parameters.externals.pLogger,
        nullptr,
        TilesetSharedAssetSystem::getDefault(),
        nullptr};

    this->_pTilesetContentManager =
        TilesetContentManager::createFromUrl(externals, TilesetOptions{}, url);
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    const RasterOverlayOptions& options = this->getOwner().getOptions();
    glm::ivec2 textureSize = glm::min(
        glm::ivec2(
            overlayTile.getTargetScreenPixels() /
            options.maximumScreenSpaceError),
        glm::ivec2(options.maximumTextureSize));

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            overlayTile.getRectangle());
    const CesiumGeospatial::Ellipsoid& ellipsoid =
        CesiumGeospatial::Ellipsoid::WGS84;

    const glm::dvec3 center =
        ellipsoid.cartographicToCartesian(tileRectangle.computeCenter());
    const glm::dvec3 dir = -glm::normalize(center);
    const double width = glm::distance(
        ellipsoid.cartographicToCartesian(tileRectangle.getNortheast()),
        ellipsoid.cartographicToCartesian(tileRectangle.getNorthwest()));
    const double height = glm::distance(
        ellipsoid.cartographicToCartesian(tileRectangle.getNortheast()),
        ellipsoid.cartographicToCartesian(tileRectangle.getSoutheast()));
    const glm::dvec3 up = glm::cross(dir, glm::dvec3(1, 0, 0));

    const ViewState viewState(
        center,
        dir,
        up,
        glm::dvec2(textureSize.x, textureSize.y),
        -(width / 2.0),
        width / 2.0,
        -(height / 2.0),
        height / 2.0);

    return this->getAsyncSystem()
        .createResolvedFuture<LoadedRasterOverlayImage>(
            LoadedRasterOverlayImage{});
  }
}
} // namespace

EmbeddedTilesetRasterOverlayTest::EmbeddedTilesetRasterOverlayTest(
    const std::string& name,
    const std::string& url,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions), _url(url) {}

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
EmbeddedTilesetRasterOverlayTest::createTileProvider(
    const CreateRasterOverlayTileProviderParameters& parameters) const {

  IntrusivePointer<const EmbeddedTilesetRasterOverlayTest> thiz = this;
  return parameters.externals.asyncSystem
      .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
          IntrusivePointer<RasterOverlayTileProvider>(
              new EmbeddedTilesetRasterOverlayTestTileProvider(
                  thiz,
                  parameters,
                  this->_url)));
}

} // namespace Cesium3DTilesSelection