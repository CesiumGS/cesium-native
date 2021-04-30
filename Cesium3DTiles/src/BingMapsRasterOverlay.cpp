#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/Uri.h"
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <utility>
#include <vector>

namespace {
struct CoverageArea {
  CesiumGeospatial::GlobeRectangle rectangle;
  uint32_t zoomMin;
  uint32_t zoomMax;
};

struct CreditAndCoverageAreas {
  Cesium3DTiles::Credit credit;
  std::vector<CoverageArea> coverageAreas;
};

struct SessionCacheItem {
  std::string metadataUrl;
  std::vector<std::byte> responseData;
};

std::vector<SessionCacheItem> sessionCache;
} // namespace

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTiles {

const std::string BingMapsStyle::AERIAL = "Aerial";
const std::string BingMapsStyle::AERIAL_WITH_LABELS = "AerialWithLabels";
const std::string BingMapsStyle::AERIAL_WITH_LABELS_ON_DEMAND =
    "AerialWithLabelsOnDemand";
const std::string BingMapsStyle::ROAD = "Road";
const std::string BingMapsStyle::ROAD_ON_DEMAND = "RoadOnDemand";
const std::string BingMapsStyle::CANVAS_DARK = "CanvasDark";
const std::string BingMapsStyle::CANVAS_LIGHT = "CanvasLight";
const std::string BingMapsStyle::CANVAS_GRAY = "CanvasGray";
const std::string BingMapsStyle::ORDNANCE_SURVEY = "OrdnanceSurvey";
const std::string BingMapsStyle::COLLINS_BART = "CollinsBart";

const std::string BingMapsRasterOverlay::BING_LOGO_HTML =
    "<a href=\"http://www.bing.com\"><img "
    "src=\"data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAAH0AAAAbCAMAAABm83BQAAAASFBMVEVHcEz////"
    "//////////////////////////////////////////////////////////////////////////"
    "/////////////8FevL4AAAAF3RSTlMAkOregA8v82/"
    "RHwZAxGBLn6q3ViY5GFKj09UAAAKPSURBVHja7dXZbqswEAZg77vBCzDv/"
    "6bH9oi4CVHUKGp7c/4L7IDxp4xHgnChyN8FAKw4yB8Fegy55L/"
    "+hzpNLXSRfW60+WVdwwgLbZ4h/7qunKoRmCTEGfeDun+q96J7ButPn3tM4aneEu91p/"
    "w5leqxIHLf+"
    "1Ovdnne2oIiZ7wKbq48broBYFQ90z2H9obQpl9q5QBa4LYFAPhe9LmhDYJFxhWpzFpc5AXjkdm"
    "tTZUmhtmoo8MublN66scYudgedU8hkQZD0ylkyCYBLB1nEIWwmsNNL9m3lXzlqmGsV6yWNvXJD"
    "l3ktlJmLtt91iC53rouA8YaN/"
    "WSC9PCT30cgoG+WwHarj7D1LnvA9d7H0Tu9cUqg2o6FKy+FU0T9z0f4Japj9g69Tze1+"
    "DJBmhJPXXcMtkxrGPA2Nr1gD+"
    "MbTq914l9ojt5KKFhuemG9HCQxADFVXbq2J0ijSHws9OqYEvXPd7YdXuol3vd19N/"
    "6PkV2E1fkAGJN3ri1APq9Iu+RG0L5cvoOoyDvim3deoj4qrj6K46/Zae4ir7mq/6zkZJDI/"
    "ui+4yXHU85uOiz8qzF7rSEo/qa+UXi6Ms8aZLoeG5vgAnV10B83guL/"
    "SK0AZDX3FZFASj4NQXBmfo1JXbAgVYrnq/FNnLqF/"
    "oAf97wa6z40fVByF+rGCop9nwZXv8xmlDnukbAx0tpJfnHst+"
    "hCjw3AWvxyZ0L0EW6li5IXAXu16+7+"
    "YYBU7tSU077p987xNqedvWwnGuV2OoyxhU6mdLOYsrEWF0XY2M5W1sVxhr797pzHjydrQm38jQ"
    "r4EZKsn7qVA+0Fc+D/ytOIedA+EDnXjD8MDfDIWYercK8oE+WuPtA8fGAWA5kA/0fwpdN1P/"
    "sv/SAAAAAElFTkSuQmCC\" title=\"Bing Imagery\"/></a>";

class BingMapsTileProvider final : public RasterOverlayTileProvider {
public:
  BingMapsTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      Credit bingCredit,
      const std::vector<CreditAndCoverageAreas>& perTileCredits,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const std::vector<std::string>& subdomains,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      const std::string& culture)
      : RasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            bingCredit,
            pPrepareRendererResources,
            pLogger,
            WebMercatorProjection(),
            QuadtreeTilingScheme(
                WebMercatorProjection::computeMaximumProjectedRectangle(
                    Ellipsoid::WGS84),
                2,
                2),
            WebMercatorProjection::computeMaximumProjectedRectangle(
                Ellipsoid::WGS84),
            minimumLevel,
            maximumLevel,
            width,
            height),
        _credits(perTileCredits),
        _urlTemplate(urlTemplate),
        _subdomains(subdomains) {
    if (this->_urlTemplate.find("n=z") == std::string::npos) {
      this->_urlTemplate =
          CesiumUtility::Uri::addQuery(this->_urlTemplate, "n", "z");
    }

    std::string resolvedUrl =
        CesiumUtility::Uri::resolve(baseUrl, this->_urlTemplate);

    resolvedUrl = CesiumUtility::Uri::substituteTemplateParameters(
        resolvedUrl,
        [&culture](const std::string& templateKey) {
          if (templateKey == "culture") {
            return culture;
          }

          // Keep other placeholders
          return "{" + templateKey + "}";
        });
  }

  virtual ~BingMapsTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const override {
    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        this->_urlTemplate,
        [this, &tileID](const std::string& key) {
          if (key == "quadkey") {
            return BingMapsTileProvider::tileXYToQuadKey(
                tileID.level,
                tileID.x,
                tileID.computeInvertedY(this->getTilingScheme()));
          }
          if (key == "subdomain") {
            size_t subdomainIndex =
                (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
            return this->_subdomains[subdomainIndex];
          }
          return key;
        });

    // Cesium levels start at 0, Bing levels start at 1
    unsigned int bingTileLevel = tileID.level + 1;
    CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            this->getTilingScheme().tileToRectangle(tileID));

    LoadTileImageFromUrlOptions options;
    options.allowEmptyImages = true;
    std::vector<Credit>& tileCredits = options.credits;

    for (const CreditAndCoverageAreas& creditAndCoverageAreas : _credits) {
      for (CoverageArea coverageArea : creditAndCoverageAreas.coverageAreas) {
        if (coverageArea.zoomMin <= bingTileLevel &&
            bingTileLevel <= coverageArea.zoomMax &&
            coverageArea.rectangle.intersect(tileRectangle).has_value()) {
          tileCredits.push_back(creditAndCoverageAreas.credit);
          break;
        }
      }
    }

    return this->loadTileImageFromUrl(url, {}, options);
  }

private:
  static std::string tileXYToQuadKey(uint32_t level, uint32_t x, uint32_t y) {
    std::string quadkey;
    for (int32_t i = static_cast<int32_t>(level); i >= 0; --i) {
      uint32_t bitmask = static_cast<uint32_t>(1 << i);
      uint32_t digit = 0;

      if ((x & bitmask) != 0) {
        digit |= 1;
      }

      if ((y & bitmask) != 0) {
        digit |= 2;
      }

      quadkey += std::to_string(digit);
    }

    return quadkey;
  }

  std::vector<CreditAndCoverageAreas> _credits;
  std::string _urlTemplate;
  std::vector<std::string> _subdomains;
};

BingMapsRasterOverlay::BingMapsRasterOverlay(
    const std::string& url,
    const std::string& key,
    const std::string& mapStyle,
    const std::string& culture,
    const Ellipsoid& ellipsoid)
    : _url(url),
      _key(key),
      _mapStyle(mapStyle),
      _culture(culture),
      _ellipsoid(ellipsoid) {}

BingMapsRasterOverlay::~BingMapsRasterOverlay() {}

namespace {

/**
 * @brief Collects credit information from an imagery metadata response.
 *
 * The imagery metadata response contains a `resourceSets` array,
 * each containing an `resources` array, where each resource has
 * the following structure:
 * \code
 * {
 *   "imageryProviders": [
 *     {
 *       "attribution": "� 2021 Microsoft Corporation",
 *       "coverageAreas": [
 *         { "bbox": [-90, -180, 90, 180], "zoomMax": 21, "zoomMin": 1 }
 *       ]
 *     },
 *     ...
 *   ],
 *   ...
 * }
 * \endcode
 *
 * @param pResource The JSON value for the resource
 * @param pCreditSystem The `CreditSystem` that will create one credit for
 * each attribution
 * @return The `CreditAndCoverageAreas` objects that have been parsed
 */
std::vector<CreditAndCoverageAreas> collectCredits(
    const rapidjson::Value* pResource,
    const std::shared_ptr<CreditSystem>& pCreditSystem) {
  std::vector<CreditAndCoverageAreas> credits;
  auto attributionsIt = pResource->FindMember("imageryProviders");
  if (attributionsIt != pResource->MemberEnd() &&
      attributionsIt->value.IsArray()) {

    for (const rapidjson::Value& attribution :
         attributionsIt->value.GetArray()) {

      std::vector<CoverageArea> coverageAreas;
      auto coverageAreasIt = attribution.FindMember("coverageAreas");
      if (coverageAreasIt != attribution.MemberEnd() &&
          coverageAreasIt->value.IsArray()) {

        for (const rapidjson::Value& area : coverageAreasIt->value.GetArray()) {

          auto bboxIt = area.FindMember("bbox");
          if (bboxIt != area.MemberEnd() && bboxIt->value.IsArray() &&
              bboxIt->value.Size() == 4) {

            auto zoomMinIt = area.FindMember("zoomMin");
            auto zoomMaxIt = area.FindMember("zoomMax");
            auto bboxArrayIt = bboxIt->value.GetArray();
            if (zoomMinIt != area.MemberEnd() &&
                zoomMaxIt != area.MemberEnd() && zoomMinIt->value.IsUint() &&
                zoomMaxIt->value.IsUint() && bboxArrayIt[0].IsNumber() &&
                bboxArrayIt[1].IsNumber() && bboxArrayIt[2].IsNumber() &&
                bboxArrayIt[3].IsNumber()) {
              CoverageArea coverageArea{

                  CesiumGeospatial::GlobeRectangle::fromDegrees(
                      bboxArrayIt[1].GetDouble(),
                      bboxArrayIt[0].GetDouble(),
                      bboxArrayIt[3].GetDouble(),
                      bboxArrayIt[2].GetDouble()),
                  zoomMinIt->value.GetUint(),
                  zoomMaxIt->value.GetUint()};

              coverageAreas.push_back(coverageArea);
            }
          }
        }
      }

      auto creditString = attribution.FindMember("attribution");
      if (creditString != attribution.MemberEnd() &&
          creditString->value.IsString()) {
        credits.push_back(
            {pCreditSystem->createCredit(creditString->value.GetString()),
             coverageAreas});
      }
    }
  }
  return credits;
}

} // namespace

Future<std::unique_ptr<RasterOverlayTileProvider>>
BingMapsRasterOverlay::createTileProvider(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {
  std::string metadataUrl = CesiumUtility::Uri::resolve(
      this->_url,
      "REST/v1/Imagery/Metadata/" + this->_mapStyle,
      true);
  metadataUrl =
      CesiumUtility::Uri::addQuery(metadataUrl, "incl", "ImageryProviders");
  metadataUrl = CesiumUtility::Uri::addQuery(metadataUrl, "key", this->_key);
  metadataUrl = CesiumUtility::Uri::addQuery(metadataUrl, "uriScheme", "https");

  pOwner = pOwner ? pOwner : this;

  auto handleResponse = 
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           pLogger,
           baseUrl = this->_url,
           culture = this->_culture]
            (const std::byte* responseBuffer, size_t responseSize)
            -> std::unique_ptr<RasterOverlayTileProvider> {
    
      rapidjson::Document response;
      response.Parse(
          reinterpret_cast<const char*>(responseBuffer),
          responseSize);

      if (response.HasParseError()) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Error when parsing Bing Maps imagery metadata, error code "
            "{} at byte offset {}",
            response.GetParseError(),
            response.GetErrorOffset());
        return nullptr;
      }

      rapidjson::Value* pResource =
          rapidjson::Pointer("/resourceSets/0/resources/0").Get(response);
      if (!pResource) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Resources were not found in the Bing Maps imagery metadata "
            "response.");
        return nullptr;
      }

      uint32_t width =
          JsonHelpers::getUint32OrDefault(*pResource, "imageWidth", 256U);
      uint32_t height = JsonHelpers::getUint32OrDefault(
          *pResource,
          "imageHeight",
          256U);
      uint32_t maximumLevel =
          JsonHelpers::getUint32OrDefault(*pResource, "zoomMax", 30U);

      std::vector<std::string> subdomains =
          JsonHelpers::getStrings(*pResource, "imageUrlSubdomains");
      std::string urlTemplate = JsonHelpers::getStringOrDefault(
          *pResource,
          "imageUrl",
          std::string());
      if (urlTemplate.empty()) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Bing Maps tile imageUrl is missing or empty.");
        return nullptr;
      }

      std::vector<CreditAndCoverageAreas> credits =
          collectCredits(pResource, pCreditSystem);
      Credit bingCredit = pCreditSystem->createCredit(BING_LOGO_HTML);

      return std::make_unique<BingMapsTileProvider>(
          *pOwner,
          asyncSystem,
          pAssetAccessor,
          bingCredit,
          credits,
          pPrepareRendererResources,
          pLogger,
          baseUrl,
          urlTemplate,
          subdomains,
          width,
          height,
          0,
          maximumLevel,
          culture);
  };

  auto cacheResultIt = std::find_if(
    sessionCache.begin(), 
    sessionCache.end(), 
    [metadataUrl](const SessionCacheItem& item) -> bool {
      return item.metadataUrl == metadataUrl;
    }
  );

  if (cacheResultIt != sessionCache.end()) {
    SPDLOG_LOGGER_ERROR(
      pLogger,
      "---------REUSING BING SESSION------");
    return asyncSystem.createResolvedFuture(
      handleResponse(
        cacheResultIt->responseData.data(),
        cacheResultIt->responseData.size())
    );
  }

  return pAssetAccessor->requestAsset(asyncSystem, metadataUrl)
      .thenInMainThread(
          [metadataUrl,
           pLogger,
           handleResponse](const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlayTileProvider> {
            const IAssetResponse* pResponse = pRequest->response();

            if (pResponse == nullptr) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Bing Maps imagery metadata "
                  "service.");
              return nullptr;
            }

            const std::byte* responseBuffer = pResponse->data().data();
            size_t responseSize = pResponse->data().size();

            SessionCacheItem& item = sessionCache.emplace_back();
            item.metadataUrl = metadataUrl;
            item.responseData.resize(responseSize);
            std::memcpy(
              item.responseData.data(), 
              responseBuffer, 
              responseSize);

            return handleResponse(responseBuffer, responseSize);
          });

}

} // namespace Cesium3DTiles
