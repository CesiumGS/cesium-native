#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>

namespace Cesium3DTilesSelection {
class TilesetLoaderFactory {
public:
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid) const = 0;

  virtual bool isValid() const = 0;
};

class CesiumIonTilesetLoaderFactory : public TilesetLoaderFactory {
public:
  CesiumIonTilesetLoaderFactory(
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl)
      : _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken),
        _ionAssetEndpointUrl(ionAssetEndpointUrl) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid) const override;

  virtual bool isValid() const override { return this->_ionAssetID > 0; }

  uint32_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
};

class ITwinCesiumCuratedContentLoaderFactory : public TilesetLoaderFactory {
public:
  ITwinCesiumCuratedContentLoaderFactory(
      uint32_t iTwinCesiumContentID,
      const std::string& iTwinAccessToken)
      : _iTwinCesiumContentID(iTwinCesiumContentID),
        _iTwinAccessToken(iTwinAccessToken) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetContentOptions& contentOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener,
      bool showCreditsOnScreen,
      const CesiumGeospatial::Ellipsoid& ellipsoid) const override;

  virtual bool isValid() const override {
    return this->_iTwinCesiumContentID > 0;
  }

  uint32_t _iTwinCesiumContentID;
  std::string _iTwinAccessToken;
};
} // namespace Cesium3DTilesSelection