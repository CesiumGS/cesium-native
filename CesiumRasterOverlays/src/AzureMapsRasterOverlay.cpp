#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumRasterOverlays/AzureMapsRasterOverlay.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <rapidjson/document.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {
std::unordered_map<std::string, std::vector<std::byte>> sessionCache;
} // namespace

namespace CesiumRasterOverlays {
const std::string AZURE_MAPS_LOGO_HTML =
    "<img alt=\"Microsoft Azure\" "
    "src=\"data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAM0AAAAnCAYAAACsY8yNAAAAAXNSR0IB2cksfwAAAAlwSFlzAA"
    "ALEwAACxMBAJqcGAAACaFJREFUeJztWweMVUUU3QUUUURBUIqBFZBFUbGgQQFDlCi2KAr2Hl2w"
    "oRRFxQgaVDYrxIINBCyASrHFFRCU2KMIlgixF4wVdBELosD1HuaO7/757/333vJ1/"
    "8c5ycnfd+fOvGl35s6dtyUlHh4eHh4eHh4eHh4eHh4e/2/8ckL7mWm4pl/"
    "7GT+d2q7lmLklZ4x6rmRmGt4wv+"
    "TYum6vh8dmgw2B0pCNZh0bze5sNJVsCJSSV9Z1ez08NhveaDw8UsIbjYdHSnij2TJBRE2ZBzB7"
    "MTvVdX22KGzpRsMTphVzMLPDf/"
    "3uugK39RLmV8yNZPBVXddpi0IhGQ0P7vHMvxRXMJvFtYF1Zjr5hom8DfM7mTjfM1vnq98KFdzG"
    "E5l/Uia+yVPZw1Uf/46dLB/"
    "lFh0KzGj6UTb6x+"
    "RpK4OocaWk7cfcILL1zL3z2XeFBm5fPebr0l7sMmczd2R2ZjZg7sMcz5xci7KR/y2nnyv/"
    "jXYUPIrAaGZjMuTIMyEkjzWarZnXM99g3oTnfPdfIYHMOeYb6YP3nbRy5jpJW1CLsrsx10p+"
    "uxAtYzbKXwuKBAVsNHaA4FZ1idBvwvxC9Fa7RvN/"
    "A7e7BfMH6YNFTloX1T+1MZp7Vf5n5RduYI/8taBIUMBGM40C33xchP4pauW71TUa/"
    "m3IPJNZwTyN2cDJD5ejB3MM80HmI8xK5uFWV95RIb9wf/"
    "qKzlXMrVRZcH2GMCcxHybjBp3D3C6i7nsyR4rudCmzd4jetlL3KtGdLO/"
    "e39E7nzmMuUb64EOpt+Vo1T/LlbxrrjGRsrdn/"
    "ip5n8O7tQFF5NnLeX8onT4GG4a036adpOTHKDl22AOZN0o/"
    "7qr0OjCHMu8m45VcziyLa3NOFLDRnM58W/"
    "7+lFnf0S1lzpH0xWQiRhbWaODPfy2yz0lNYP67GXMq82fKxk/"
    "MtqL3ocg+Zl6kJs+m8pjbMK+jIOCggXPUIuYhTt37q3ppzHf0Dma+TNkHe2AVGSPdVnR/"
    "C9FJgthdmTLH5Swyi80KeYZHsFNInssTvHut08dAU6ecVirtHSVfoOSXkvFILHqKDuqqo4gWH5"
    "EywNQoYKPBSjJcPR/q6JZRMOHRaYmNhswONIcy8RnzRSkTE6G9M6BrKXAZ/"
    "ymPeZdTzifMJZQ50b8lWd3IBC5+FDkCGLOYj4lsgWpfRzLGa/"
    "EHmbPZCiXDDlsl+v+m0cwSXdSxlchuUmVUhOTBmKwLoZ3A+"
    "B3o9DFQG6OpcdrUk8wCZwNEcN3hXt5PwbigLgfGtT0UP/"
    "ffbUoarh5QNnHVGW13GVtdOmDMvJIpKXlUzOC4RrM7BSs7ttd6Sne8yDGRd6Z0RoMVer3IsRL1"
    "sGWTcQcGosyQAYXu4TKQuDTcV5WD95yo6gejfkLlnSjyQUo2QunDkHvL3whgLFJ69zCbS1p9yt"
    "zxYOS7Sb3bMVeK/CUyRm3ZTZX3gpL/"
    "42JGjElz9S6cZUpFrscm0RmJ9Y6mIBhRTRKYoc03Giwu8Exaow/JuI/"
    "2bIdFpovK14OCxfahJPXOxvSaNmlZb9qqBmvm1G/"
    "y+5OlrVOycUynukZTSsGhEx1TLnqNKPDdbxVZGqNZLjIM+"
    "pExdbIDmnXo5ecH1DvDVttmFKxsG6Q+2m2ZSM45S/IhVG6NETtXYycd/"
    "VKpyhkk8rwHAihzRznPqcNLqm05vzogs4jYuqFPy1Ta5hrNSCdPhUrrF1KX+ZJWy/"
    "ur6TWrU3Ils+"
    "Nfz5SO3ji3pCYlL4vp2AyjERlWELuljxDZafKMVaubyBIZjchsyPQVCpm0Tp3sgGLFaqLk2A0+"
    "UPXoHJF/iapXdzIBALvSbZA6XEiyk0ies1SeaRHlHq107hVZXo2GzOK0TPLA/"
    "cMq3UbxFlXmhBzlNCZzdWD7qndEHwO1MRrXddeRPgRZ7nP4iaRtTNIP2ZheQyn5J7PT+"
    "uqSSprH9UvHVGcakbWkYHK+SuYQ+"
    "qQ8v8fcRvSSGk250ns0rnvUgOLwuLWS70DGXQPgM7eJyD9Pva8PmRUad0f6QhaLAi4Ou0qewSo"
    "tKnJ4kNKZLbJ8Gw1cOrtTor5fSj9a6sM3AiVRkcJrVXun5OhjoDZG08XJM5sSIkk/"
    "ZKPAjUbkQ0WGAcQtt3VdhimdpEbTUelVx3UPRRsNVs8vJA2rcFlE/"
    "lfU+7oreVcyQYRfVToCBjsxL1ay+yLKPczVofwbzXhKDhh+"
    "35AysDtZg3mNHFfT6WPANZrWKi2p0Tyi0hBMGhzFJP2QjeIwmnYUrHir5BeTbQ+"
    "lk9RocGC2h1FM+"
    "rKYOkUZDe5sXo2ZMA1VfTFxWjjpONTDZXtX1f08ynS9MNHCzj1DlM4lIktqNAtztVn0cR6zrhm"
    "CLaMjOEOVe49TBiKFb0oaIm/"
    "7RrxruSqjzEk7QKUlNRp93su4z8oPisBoJG0+"
    "ZQIumo6mpQkEVCtdBBrcS08cxJvK36FGI2nnqnKwo7RQaTAqHTKfJvJDyPkIlZ+"
    "vVnowBkQD7eSHsZ3s6MMA7C6HxaOTyHMZTWf1jo9yjYPoX0DBjj43h14zpYdzXzuRY0F4XL0z8"
    "jzLaQuV3mAnbZJKS2o0fVXapkhhyDsRFcy6X0qG4jEaHI713UdPJz2N0eDSUF9GYqWHK4KV82n"
    "mL5R9TxNmNLjY1KskJnKVlIOzjJ1M2G06Sx58BQA3bILUeQQFB1PsgDZCOFCVC/"
    "cPExBnodskvwVC8TYMnMtosMvqeyYckPElROglH5mbf4uLY8btedHDjjtcZOeq9q+R+"
    "s8IIXbj8ykI9qDvx5H5uuExyryYTGo08CYWq/"
    "S3pJ8RVELIfyqZYEzo51nxKB6jwQS1W33WSknpvwg4lII7DRf47D3WaCQddyTLIsoBECzoo/"
    "RHRughkjbcKRufhYR9DWD1MbkaKf1Io5F0HMjd2/GsMSETfNFfh+f8lwrKNPClInO/PI/"
    "Cprsi5lMRbbxNPScyGtUXb1M00K7yXO2KRmEZDb5XqhJmhXBZdpykZa2OZNwem7e3yBAyvU5k1"
    "1D2TlEucqyqCA3j/PAQmW+h7MXbVZIfv/"
    "Xd94oOImmYODiALpaycLEJt6tVyDtvJuNuLiXj1iEMegQ5X3OTcfGwK95OZjXHJMDdyB1kXBD3"
    "0yIEJ0ZJfQeF1BP9MVTKQh2rKfx7t71VX14R1mZHv6XSHyvvqUpI28/oQ5zn8A0gvta4U/"
    "qkqdIdot5ZoeS7RNQLruOlUh7GBTvOM9L/"
    "vaLGMx4FZDQeHsUBbzQeHinhjcbDIyW80Xh4pIQ3Gg8PDw8PDw8PDw8PDw8PDw8PDw8Pj9T4G6"
    "U/URjRT3IrAAAAAElFTkSuQmCC\"/>";

namespace {
Rectangle createRectangle(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner) {
  return WebMercatorProjection::computeMaximumProjectedRectangle(
      pOwner->getOptions().ellipsoid);
}

QuadtreeTilingScheme createTilingScheme(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner) {
  return QuadtreeTilingScheme(createRectangle(pOwner), 1, 1);
}
} // namespace

class AzureMapsRasterOverlayTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  AzureMapsRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& baseUrl,
      const std::string& apiVersion,
      const std::string& tilesetId,
      const std::string& key,
      const std::string& tileEndpoint,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t imageSize,
      bool showLogo);

  virtual ~AzureMapsRasterOverlayTileProvider() = default;

  void update(const AzureMapsRasterOverlayTileProvider& newProvider) {
    this->_baseUrl = newProvider._baseUrl;
    this->_apiVersion = newProvider._apiVersion;
    this->_tilesetId = newProvider._tilesetId;
    this->_key = newProvider._key;
    this->_tileEndpoint = newProvider._tileEndpoint;
  }

  CesiumAsync::Future<void> loadCredits();

  virtual void addCredits(
      CesiumUtility::CreditReferencer& creditReferencer) noexcept override;

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override;

private:
  std::string _baseUrl;
  std::string _apiVersion;
  std::string _tilesetId;
  std::string _key;
  std::string _tileEndpoint;
  std::optional<Credit> _azureCredit;
  std::vector<Credit> _credits;
  bool _showCreditsOnScreen;
};

AzureMapsRasterOverlay::AzureMapsRasterOverlay(
    const std::string& name,
    const AzureMapsSessionParameters& sessionParameters,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _sessionParameters(sessionParameters) {
  Uri::ensureTrailingSlash(this->_sessionParameters.apiBaseUrl);
}

AzureMapsRasterOverlay::~AzureMapsRasterOverlay() = default;

Future<RasterOverlay::CreateTileProviderResult>
AzureMapsRasterOverlay::createTileProvider(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    IntrusivePointer<const RasterOverlay> pOwner) const {
  Uri tilesetUri(this->_sessionParameters.apiBaseUrl, "map/tileset");

  UriQuery tilesetQuery(tilesetUri);
  tilesetQuery.setValue("api-version", this->_sessionParameters.apiVersion);
  tilesetQuery.setValue("tilesetId", this->_sessionParameters.tilesetId);
  tilesetQuery.setValue("subscription-key", this->_sessionParameters.key);
  tilesetQuery.setValue("language", this->_sessionParameters.language);
  tilesetQuery.setValue("view", this->_sessionParameters.view);
  tilesetUri.setQuery(tilesetQuery.toQueryString());

  std::string tilesetUrl = std::string(tilesetUri.toString());

  pOwner = pOwner ? pOwner : this;

  auto handleResponse =
      [pOwner,
       asyncSystem,
       pAssetAccessor,
       pCreditSystem,
       pPrepareRendererResources,
       pLogger,
       &sessionParameters = this->_sessionParameters](
          const std::shared_ptr<IAssetRequest>& pRequest,
          const std::span<const std::byte>& data) -> CreateTileProviderResult {
    JsonObjectJsonHandler handler{};
    ReadJsonResult<JsonValue> response = JsonReader::readJson(data, handler);

    if (!response.value) {
      ErrorList errorList{};
      errorList.errors = std::move(response.errors);
      errorList.warnings = std::move(response.warnings);

      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = errorList.format("Failed to parse response from Azure "
                                      "Maps tileset API service:")});
    }

    if (!response.value->isObject()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service was not a "
                     "JSON object."});
    }
    const JsonValue::Object& responseObject = response.value->getObject();

    uint32_t tileSize = 256u;
    uint32_t minimumLevel = 0u;
    uint32_t maximumLevel = 22u;

    // "tileSize" is an optional enum property that is delivered as a string.
    std::string tileSizeEnum = "256";
    auto it = responseObject.find("tileSize");
    if (it != responseObject.end() && it->second.isString()) {
      tileSizeEnum = it->second.getString();
    }

    try {
      tileSize = static_cast<uint32_t>(std::stoul(tileSizeEnum));
    } catch (std::exception&) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'tileSize' property."});
    }

    it = responseObject.find("minzoom");
    if (it != responseObject.end() && it->second.isNumber()) {
      minimumLevel = it->second.getSafeNumberOrDefault<uint32_t>(0);
    } else if (it != responseObject.end()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'minzoom' property."});
    }

    it = responseObject.find("maxzoom");
    if (it != responseObject.end() && it->second.isNumber()) {
      maximumLevel = it->second.getSafeNumberOrDefault<uint32_t>(0);
    } else if (it != responseObject.end()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'maxzoom' property."});
    }

    std::vector<std::string> tiles;
    it = responseObject.find("tiles");
    if (it != responseObject.end()) {
      tiles = it->second.getArrayOfStrings(std::string());
    }

    if (tiles.empty()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'tiles' property."});
    }

    std::string tileEndpoint;
    for (const std::string& endpoint : tiles) {
      // Azure Maps documentation: "If multiple endpoints are specified, clients
      // may use any combination of endpoints. All endpoints MUST return the
      // same content for the same URL."
      // This just picks the first non-empty string endpoint.
      if (!endpoint.empty()) {
        tileEndpoint = endpoint;
        break;
      }
    }

    if (tileEndpoint.empty()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          "Azure Maps returned no valid endpoints for the given tilesetId."});
    }

    std::optional<Credit> topLevelCredit;

    it = responseObject.find("attribution");
    if (it != responseObject.end() && it->second.isString()) {
      topLevelCredit = pCreditSystem->createCredit(
          it->second.getString(),
          pOwner->getOptions().showCreditsOnScreen);
    }

    auto* pProvider = new AzureMapsRasterOverlayTileProvider(
        pOwner,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        topLevelCredit,
        pPrepareRendererResources,
        pLogger,
        sessionParameters.apiBaseUrl,
        sessionParameters.apiVersion,
        sessionParameters.tilesetId,
        sessionParameters.key,
        tileEndpoint,
        minimumLevel,
        maximumLevel,
        tileSize,
        sessionParameters.showLogo);

    // Start loading credits, but don't wait for the load to finish.
    pProvider->loadCredits();

    return pProvider;
  };

  auto cacheResultIt = sessionCache.find(tilesetUrl);
  if (cacheResultIt != sessionCache.end()) {
    return asyncSystem.createResolvedFuture(
        handleResponse(nullptr, std::span<std::byte>(cacheResultIt->second)));
  }

  return pAssetAccessor->get(asyncSystem, tilesetUrl)
      .thenInMainThread(
          [tilesetUrl,
           handleResponse](std::shared_ptr<IAssetRequest>&& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();

            if (!pResponse) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  pRequest,
                  "No response received from Azure Maps imagery tileset "
                  "service."});
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::TileProvider,
                  .pRequest = pRequest,
                  .message = fmt::format(
                      "Azure Maps API tileset request service returned "
                      "HTTP status code {}.",
                      pResponse->statusCode())});
            }

            CreateTileProviderResult handleResponseResult =
                handleResponse(pRequest, pResponse->data());

            // If the response successfully created a tile provider, cache it.
            if (handleResponseResult) {
              sessionCache[tilesetUrl] = std::vector<std::byte>(
                  pResponse->data().begin(),
                  pResponse->data().end());
            }

            return handleResponseResult;
          });
}

Future<void> AzureMapsRasterOverlay::refreshTileProviderWithNewKey(
    const IntrusivePointer<RasterOverlayTileProvider>& pProvider,
    const std::string& newKey) {
  this->_sessionParameters.key = newKey;

  return this
      ->createTileProvider(
          pProvider->getAsyncSystem(),
          pProvider->getAssetAccessor(),
          pProvider->getCreditSystem(),
          pProvider->getPrepareRendererResources(),
          pProvider->getLogger(),
          &pProvider->getOwner())
      .thenInMainThread([pProvider](CreateTileProviderResult&& result) {
        if (!result) {
          SPDLOG_LOGGER_WARN(
              pProvider->getLogger(),
              "Could not refresh Azure Maps raster overlay with a new key: {}.",
              result.error().message);
          return;
        }

        // Use static_cast instead of dynamic_cast here to avoid the need for
        // RTTI, and because we are certain of the type.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
        AzureMapsRasterOverlayTileProvider* pOld =
            static_cast<AzureMapsRasterOverlayTileProvider*>(pProvider.get());
        AzureMapsRasterOverlayTileProvider* pNew =
            static_cast<AzureMapsRasterOverlayTileProvider*>(result->get());
        // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
        if (pOld->getCoverageRectangle().getLowerLeft() !=
                pNew->getCoverageRectangle().getLowerLeft() ||
            pOld->getCoverageRectangle().getUpperRight() !=
                pNew->getCoverageRectangle().getUpperRight() ||
            pOld->getHeight() != pNew->getHeight() ||
            pOld->getWidth() != pNew->getWidth() ||
            pOld->getMinimumLevel() != pNew->getMinimumLevel() ||
            pOld->getMaximumLevel() != pNew->getMaximumLevel() ||
            pOld->getProjection() != pNew->getProjection()) {
          SPDLOG_LOGGER_WARN(
              pProvider->getLogger(),
              "Could not refresh Azure Maps raster overlay with a new key "
              "because some metadata properties changed unexpectedly upon "
              "refresh.");
          return;
        }

        pOld->update(*pNew);
      });
}

namespace {
CesiumAsync::Future<rapidjson::Document> fetchAttributionData(
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& apiBaseUrl,
    const std::string& apiVersion,
    const std::string& tilesetId,
    const std::string& key,
    uint32_t zoom,
    const Rectangle& rectangleDegrees) {
  std::vector<std::string> bounds(4);
  bounds[0] = std::to_string(rectangleDegrees.minimumX); // west
  bounds[1] = std::to_string(rectangleDegrees.minimumY); // south
  bounds[2] = std::to_string(rectangleDegrees.maximumX); // east
  bounds[3] = std::to_string(rectangleDegrees.maximumY); // north

  Uri attributionUri(apiBaseUrl, "map/attribution", true);

  UriQuery query(attributionUri);
  query.setValue("api-version", apiVersion);
  query.setValue("subscription-key", key);
  query.setValue("tilesetId", tilesetId);
  query.setValue("zoom", std::to_string(zoom));
  query.setValue("bounds", joinToString(bounds, ","));
  attributionUri.setQuery(query.toQueryString());

  return pAssetAccessor
      ->get(asyncSystem, std::string(attributionUri.toString()))
      .thenInMainThread(
          [pLogger](const std::shared_ptr<IAssetRequest>& pRequest)
              -> rapidjson::Document {
            const IAssetResponse* pResponse = pRequest->response();
            rapidjson::Document document;

            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Azure Maps API attribution "
                  "service.");
              return document;
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error response {} received from Azure Maps API attribution "
                  "service URL {}.",
                  pResponse->statusCode(),
                  pRequest->url());
              return document;
            }

            document.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            if (document.HasParseError()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error when parsing Azure Maps API attribution service JSON "
                  "from URL {}, error code {} at byte offset {}.",
                  pRequest->url(),
                  document.GetParseError(),
                  document.GetErrorOffset());
            }

            return document;
          });
}
} // namespace

AzureMapsRasterOverlayTileProvider::AzureMapsRasterOverlayTileProvider(
    const IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    std::optional<CesiumUtility::Credit> credit,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const std::string& apiVersion,
    const std::string& tilesetId,
    const std::string& key,
    const std::string& tileEndpoint,
    uint32_t minimumLevel,
    uint32_t maximumLevel,
    uint32_t imageSize,
    bool showLogo)
    : QuadtreeRasterOverlayTileProvider(
          pOwner,
          asyncSystem,
          pAssetAccessor,
          pCreditSystem,
          credit,
          pPrepareRendererResources,
          pLogger,
          WebMercatorProjection(pOwner->getOptions().ellipsoid),
          createTilingScheme(pOwner),
          createRectangle(pOwner),
          minimumLevel,
          maximumLevel,
          imageSize,
          imageSize),
      _baseUrl(baseUrl),
      _apiVersion(apiVersion),
      _tilesetId(tilesetId),
      _key(key),
      _tileEndpoint(tileEndpoint),
      _azureCredit(),
      _credits(),
      _showCreditsOnScreen(pOwner->getOptions().showCreditsOnScreen) {
  if (pCreditSystem && showLogo) {
    this->_azureCredit =
        pCreditSystem->createCredit(AZURE_MAPS_LOGO_HTML, true);
  }
}

CesiumAsync::Future<LoadedRasterOverlayImage>
AzureMapsRasterOverlayTileProvider::loadQuadtreeTileImage(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  Uri uri(CesiumUtility::Uri::substituteTemplateParameters(
      this->_tileEndpoint,
      [this, &tileID](const std::string& key) {
        if (key == "z") {
          return std::to_string(tileID.level);
        }
        if (key == "x") {
          return std::to_string(tileID.x);
        }
        if (key == "y") {
          return std::to_string(
              tileID.computeInvertedY(this->getTilingScheme()));
        }
        return key;
      }));

  UriQuery query(uri);
  query.setValue("subscription-key", this->_key);
  uri.setQuery(query.toQueryString());

  std::string url = std::string(uri.toString());

  LoadTileImageFromUrlOptions options;
  options.allowEmptyImages = true;
  options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
  options.rectangle = this->getTilingScheme().tileToRectangle(tileID);

  return this->loadTileImageFromUrl(url, {}, std::move(options));
}

Future<void> AzureMapsRasterOverlayTileProvider::loadCredits() {
  const uint32_t maximumZoomLevel = this->getMaximumLevel();

  IntrusivePointer thiz = this;

  std::vector<Future<std::vector<std::string>>> creditFutures;
  creditFutures.reserve(maximumZoomLevel + 1);

  for (size_t i = 0; i <= maximumZoomLevel; ++i) {
    creditFutures.emplace_back(
        fetchAttributionData(
            this->getAssetAccessor(),
            this->getAsyncSystem(),
            this->getLogger(),
            this->_baseUrl,
            this->_apiVersion,
            this->_tilesetId,
            this->_key,
            uint32_t(i),
            Rectangle(-180.0, -90.0, 180.0, 90.0))
            .thenInMainThread([](rapidjson::Document&& document) {
              if (document.HasParseError() || !document.IsObject()) {
                return std::vector<std::string>();
              }
              return JsonHelpers::getStrings(document, "copyrights");
            }));
  }

  return this->getAsyncSystem()
      .all(std::move(creditFutures))
      .thenInMainThread(
          [thiz](std::vector<std::vector<std::string>>&& results) {
            std::set<std::string> uniqueCredits;
            for (size_t i = 0; i < results.size(); i++) {
              const std::vector<std::string>& credits = results[i];
              for (size_t j = 0; j < credits.size(); j++) {
                if (!credits[j].empty()) {
                  uniqueCredits.insert(credits[j]);
                }
              }
            }

            for (const std::string& credit : uniqueCredits) {
              thiz->_credits.emplace_back(thiz->getCreditSystem()->createCredit(
                  credit,
                  thiz->_showCreditsOnScreen));
            }
          });
}

void AzureMapsRasterOverlayTileProvider::addCredits(
    CesiumUtility::CreditReferencer& creditReferencer) noexcept {
  QuadtreeRasterOverlayTileProvider::addCredits(creditReferencer);

  if (this->_azureCredit) {
    creditReferencer.addCreditReference(*this->_azureCredit);
  }

  for (const Credit& credit : this->_credits) {
    creditReferencer.addCreditReference(credit);
  }
}

} // namespace CesiumRasterOverlays
