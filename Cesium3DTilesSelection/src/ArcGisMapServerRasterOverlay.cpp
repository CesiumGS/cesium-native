#include "Cesium3DTilesSelection/ArcGisMapServerRasterOverlay.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Uri.h"
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <algorithm>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

class ArcGisMapServerTileProvider final : public RasterOverlayTileProvider {
public:
  ArcGisMapServerTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      const std::optional<bool>& useTiles,
      const std::optional<bool>& isGeographicProjection,
      const std::string& layers)
      : RasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _useTiles(useTiles),
        _tileWidth(width),
        _tileHeight(height),
        _isGeographicProjection(isGeographicProjection),
        _layers(layers),
        _pLogger(pLogger){}

  virtual ~ArcGisMapServerTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const override {
    if (*this->_useTiles) {
      std::string url = CesiumUtility::Uri::resolve(
        this->_url,
        "tile/" + std::to_string(tileID.level) + "/" +
        std::to_string(tileID.computeInvertedY(this->getTilingScheme())) + "/" + std::to_string(tileID.x), true);
      //SPDLOG_LOGGER_ERROR(
      //  _pLogger,
      //  url);
      return this->loadTileImageFromUrl(url, {}, {});
    }
    else {
      CesiumGeometry::Rectangle nativeRectangle = this->getTilingScheme().tileToRectangle(tileID);
      std::string bbox =
        std::to_string(nativeRectangle.minimumX) +
        "," +
        std::to_string(nativeRectangle.minimumY) +
        "," +
        std::to_string(nativeRectangle.maximumX) +
        "," +
        std::to_string(nativeRectangle.maximumY);

      std::string url = CesiumUtility::Uri::resolve(
        this->_url,
        "export", true);

      url = CesiumUtility::Uri::addQuery(url, "bbox", bbox);
      url = CesiumUtility::Uri::addQuery(url, "size", std::to_string(this->_tileWidth) + "," + std::to_string(this->_tileWidth));
      url = CesiumUtility::Uri::addQuery(url, "format", "png32");
      url = CesiumUtility::Uri::addQuery(url, "transparent", "true");
      url = CesiumUtility::Uri::addQuery(url, "f", "image");
      if (*this->_isGeographicProjection) {
        url = CesiumUtility::Uri::addQuery(url, "bboxSR", "4326");
        url = CesiumUtility::Uri::addQuery(url, "imageSR", "4326");
      }
      else {
        url = CesiumUtility::Uri::addQuery(url, "bboxSR", "3857");
        url = CesiumUtility::Uri::addQuery(url, "imageSR", "3857");
      }
      if (!this->_layers.empty()){
        url = CesiumUtility::Uri::addQuery(url, "layers", "show:" + this->_layers);
      }

      SPDLOG_LOGGER_ERROR(
        _pLogger,
        url);
      return this->loadTileImageFromUrl(url, {}, {});
    }
  }

private:
  std::string _url;
  std::optional<bool> _useTiles;
  uint32_t _tileWidth;
  uint32_t _tileHeight;
  std::optional<bool> _isGeographicProjection;
  std::string _layers;
  const std::shared_ptr<spdlog::logger>& _pLogger;
};

ArcGisMapServerRasterOverlay::ArcGisMapServerRasterOverlay(
    const std::string& url,
    const ArcGisMapServerRasterOverlayOptions& options)
    : _url(url), _options(options) {}

ArcGisMapServerRasterOverlay::~ArcGisMapServerRasterOverlay() {}

Future<std::unique_ptr<RasterOverlayTileProvider>>
ArcGisMapServerRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {
  std::string metadataUrl = CesiumUtility::Uri::resolve(
      this->_url,
      "",
      true);
  metadataUrl = CesiumUtility::Uri::addQuery(metadataUrl, "f", "json");

  pOwner = pOwner ? pOwner : this;

  auto handleResponse =
      [pOwner,
       asyncSystem,
       pAssetAccessor,
       pCreditSystem,
       pPrepareRendererResources,
       pLogger,
       baseUrl = this->_url,
       options = this->_options](
          const std::byte* responseBuffer,
          size_t responseSize) -> std::unique_ptr<RasterOverlayTileProvider> {
      rapidjson::Document response;
      response.Parse(reinterpret_cast<const char*>(responseBuffer), responseSize);

      if (response.HasParseError()) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Error when parsing Arcgis Maps imagery metadata, error code "
            "{} at byte offset {}",
            response.GetParseError(),
            response.GetErrorOffset());
        return nullptr;
      }

      rapidjson::Value* tileInfo =
        rapidjson::Pointer("/tileInfo").Get(response);

      std::optional<bool> useTiles = true;
      std::optional<bool> isGeographicProjection = false;
      std::string layers = options.layers.value_or("");
      CesiumGeospatial::Projection projection = CesiumGeospatial::WebMercatorProjection();
      CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
        CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
      CesiumGeometry::Rectangle rectangle = projectRectangleSimple(projection, tilingSchemeRectangle);
      uint32_t rootTilesX = 1;
      uint32_t width = 256U;
      uint32_t height = 256U;
      uint32_t maximumLevel = 30U;

      if (!tileInfo) {
        useTiles = false;
      }
      else{
        width =
          JsonHelpers::getUint32OrDefault(*tileInfo, "rows", 256U);
        height =
          JsonHelpers::getUint32OrDefault(*tileInfo, "cols", 256U);

        rapidjson::Value* wkid =
          rapidjson::Pointer("/tileInfo/spatialReference/wkid").Get(response);

        if (wkid->GetInt() == 102100 || wkid->GetInt() == 102113){
          projection = CesiumGeospatial::WebMercatorProjection();
          tilingSchemeRectangle = CesiumGeospatial::
            WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
        }
        else if (wkid->GetInt() == 4326){
          projection = CesiumGeospatial::GeographicProjection();
          isGeographicProjection = true;
          tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
            MAXIMUM_GLOBE_RECTANGLE;
          rootTilesX = 2;
        }
        else{
          SPDLOG_LOGGER_ERROR(
            pLogger,
            "Tile spatial reference WKID {} is not supported.",
            wkid->GetInt());
          return nullptr;
        }
        rapidjson::Value* lods =
          rapidjson::Pointer("/tileInfo/lods").Get(response);
        maximumLevel = lods->Size() - 1;

        rectangle = projectRectangleSimple(projection, tilingSchemeRectangle);

        rapidjson::Value* fullExtent =
          rapidjson::Pointer("/fullExtent").Get(response);
        if (fullExtent) {
          rapidjson::Value* fullExtent_spatialReference =
            rapidjson::Pointer("/fullExtent/spatialReference").Get(response);
          rapidjson::Value* fullExtent_spatialReference_wkid =
            rapidjson::Pointer("/fullExtent/spatialReference/wkid").Get(response);

          rapidjson::Value* xmin = rapidjson::Pointer("/fullExtent/xmin").Get(response);
          rapidjson::Value* ymin = rapidjson::Pointer("/fullExtent/ymin").Get(response);
          rapidjson::Value* xmax = rapidjson::Pointer("/fullExtent/xmax").Get(response);
          rapidjson::Value* ymax = rapidjson::Pointer("/fullExtent/ymax").Get(response);

          if (fullExtent_spatialReference && fullExtent_spatialReference_wkid){
            if (fullExtent_spatialReference_wkid->GetInt() == 102100 || fullExtent_spatialReference_wkid->GetInt() == 102113){
              projection = CesiumGeospatial::WebMercatorProjection();
              CesiumGeospatial::Ellipsoid ellipsoid =
                 options.ellipsoid.value_or(CesiumGeospatial::Ellipsoid::WGS84);
              rectangle = CesiumGeometry::Rectangle(
                  std::max(
                      xmin->GetDouble(),
                      ellipsoid.getMaximumRadius() * Math::ONE_PI * -1),
                  std::max(
                      ymin->GetDouble(),
                      ellipsoid.getMaximumRadius() * Math::ONE_PI * -1),
                  std::min(
                      xmax->GetDouble(),
                      ellipsoid.getMaximumRadius() * Math::ONE_PI),
                  std::min(
                      ymax->GetDouble(),
                      ellipsoid.getMaximumRadius() * Math::ONE_PI)
              );
            }
            else if (fullExtent_spatialReference_wkid->GetInt() == 4326) {
              rectangle = projectRectangleSimple(
                projection,
                CesiumGeospatial::GlobeRectangle::fromDegrees(xmin->GetDouble(), ymin->GetDouble(), xmax->GetDouble(), ymax->GetDouble())
              );
            }
            else {
              SPDLOG_LOGGER_ERROR(
                pLogger,
                "fullExtent.spatialReference WKID {} is not supported.",
                fullExtent_spatialReference_wkid->GetInt());
              return nullptr;
            }
          }
        }
        else {
          rectangle = options.coverageRectangle.value();
        }

        useTiles = true;
      }

      rapidjson::Value* copyrightText =
        rapidjson::Pointer("/copyrightText").Get(response);
      std::optional<Credit> credit = options.credit ?
        std::make_optional(
          pCreditSystem->createCredit(options.credit.value()))
        : std::nullopt;;
      if (copyrightText && copyrightText->GetStringLength() > 0) {
        (copyrightText->GetString());
        credit = std::make_optional(
          pCreditSystem->createCredit(std::string(copyrightText->GetString())));
      }

      CesiumGeometry::QuadtreeTilingScheme tilingScheme(
        projectRectangleSimple(projection, tilingSchemeRectangle),
        rootTilesX,
        1);

      return std::make_unique<ArcGisMapServerTileProvider>(
        *pOwner,
        asyncSystem,
        pAssetAccessor,
        credit,
        pPrepareRendererResources,
        pLogger,
        projection,
        tilingScheme,
        rectangle,
        baseUrl,
        width,
        height,
        0,
        maximumLevel,
        useTiles,
        isGeographicProjection,
        layers);
    };
  return pAssetAccessor->requestAsset(asyncSystem, metadataUrl)
      .thenInMainThread(
          [metadataUrl, pLogger, handleResponse](
              const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlayTileProvider> {
            const IAssetResponse* pResponse = pRequest->response();

            if (pResponse == nullptr) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Arcgis Maps imagery metadata "
                  "service.");
              return nullptr;
            }

            const std::byte* responseBuffer = pResponse->data().data();
            size_t responseSize = pResponse->data().size();

            return handleResponse(responseBuffer, responseSize);
          });
  }
} // namespace Cesium3DTilesSelection
