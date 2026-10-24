#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoaderFactory.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumI3SSelection/IResourceLocator.h>
#include <CesiumI3SSelection/Library.h>

#include <memory>
#include <string>
#include <vector>

namespace CesiumI3SSelection {

/**
 * @brief Factory for constructing an \ref I3STilesetLoader from a layer URL.
 */
class CESIUMI3SSELECTION_API I3STilesetLoaderFactory
    : public Cesium3DTilesSelection::TilesetContentLoaderFactory {
public:
  /**
   * @brief Constructs the factory from a live-service URL.
   *
   * An \ref HttpResourceLocator is constructed internally from \p layerUrl.
   *
   * @param layerUrl URL to the I3S scene layer JSON (the `layers/0` endpoint).
   * @param requestHeaders Optional HTTP headers to attach to all requests.
   * @param pGeoidGrid Optional EGM96 geoid grid for height corrections.
   */
  I3STilesetLoaderFactory(
      std::string layerUrl,
      std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders = {},
      std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
          pGeoidGrid = nullptr);

  /**
   * @brief Constructs the factory from a custom resource locator.
   *
   * Use this overload to load from an SLPK file or any other custom source by
   * supplying an \ref IResourceLocator implementation.
   *
   * @param pResourceLocator Custom resource locator (must not be null).
   * @param requestHeaders Optional HTTP headers to attach to all requests.
   * @param pGeoidGrid Optional EGM96 geoid grid for height corrections.
   */
  I3STilesetLoaderFactory(
      std::shared_ptr<IResourceLocator> pResourceLocator,
      std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders = {},
      std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
          pGeoidGrid = nullptr);

  CesiumAsync::Future<Cesium3DTilesSelection::TilesetContentLoaderResult<
      Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const Cesium3DTilesSelection::TilesetExternals& externals,
      const Cesium3DTilesSelection::TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) override;

  bool isValid() const override;

private:
  std::shared_ptr<IResourceLocator> _pResourceLocator;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::shared_ptr<CesiumGeospatial::EarthGravitationalModel1996Grid>
      _pGeoidGrid;
};

} // namespace CesiumI3SSelection
