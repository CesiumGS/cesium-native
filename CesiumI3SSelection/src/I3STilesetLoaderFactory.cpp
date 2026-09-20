#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumI3SSelection/HttpResourceLocator.h>
#include <CesiumI3SSelection/I3STilesetLoader.h>
#include <CesiumI3SSelection/I3STilesetLoaderFactory.h>

using namespace Cesium3DTilesSelection;

namespace CesiumI3SSelection {

I3STilesetLoaderFactory::I3STilesetLoaderFactory(
    std::string layerUrl,
    std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders,
    std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
        pGeoidGrid)
    : _pResourceLocator(
          std::make_shared<HttpResourceLocator>(std::move(layerUrl))),
      _requestHeaders(std::move(requestHeaders)),
      _pGeoidGrid(std::move(pGeoidGrid)) {}

I3STilesetLoaderFactory::I3STilesetLoaderFactory(
    std::shared_ptr<IResourceLocator> pResourceLocator,
    std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders,
    std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
        pGeoidGrid)
    : _pResourceLocator(std::move(pResourceLocator)),
      _requestHeaders(std::move(requestHeaders)),
      _pGeoidGrid(std::move(pGeoidGrid)) {}

bool I3STilesetLoaderFactory::isValid() const {
  return this->_pResourceLocator != nullptr;
}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetContentLoader>>
I3STilesetLoaderFactory::createLoader(
    const TilesetExternals& externals,
    const TilesetOptions& /*tilesetOptions*/,
    const AuthorizationHeaderChangeListener& /*headerChangeListener*/) {
  return I3STilesetLoader::createLoader(
             externals,
             this->_pResourceLocator,
             this->_requestHeaders,
             this->_pGeoidGrid)
      .thenImmediately(
          [](TilesetContentLoaderResult<I3STilesetLoader>&& result)
              -> TilesetContentLoaderResult<TilesetContentLoader> {
            return std::move(result);
          });
}

} // namespace CesiumI3SSelection
