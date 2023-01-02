#include "Cesium3DTilesSelection/BingMapsRasterOverlay.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
struct CoverageArea {
  CesiumGeospatial::GlobeRectangle rectangle;
  uint32_t zoomMin;
  uint32_t zoomMax;
};

struct CreditAndCoverageAreas {
  Cesium3DTilesSelection::Credit credit;
  std::vector<CoverageArea> coverageAreas;
};

std::unordered_map<std::string, std::vector<std::byte>> sessionCache;
} // namespace

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

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
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAAFgAAAATCAMAAAAj1DqpAAAAq1BMVEUAAAD////"
    "//////////////////////////////////////////////////////////////////////////"
    "//////////////////////////////////////////////////////////////////////////"
    "////////////////////////////////////////////////////////////////////////"
    "Nr6iZAAAAOHRSTlMABRRJ0xkgCix/"
    "uOGFYTMmHQh7EANWTQ2s5ZSLaURBzZB0blE6MPLav6SfiD81tJpl68kW98NauovCUzcAAAJxSU"
    "RBVDjLtdTpcpswEADgFQiQzCnAYAMGzGUw+I6P93+yStC0djudZKaJ/"
    "kjaXb7RrADAsazAd4zH4xR7fP4G+HEXR/"
    "4OePYKozQog5oBqKb6pbCUbTKMcwUCXH4tvOxVizYJeJX3nzApZM9/"
    "gtcAl02PUIhAY8gho48UchERsWTMtjxAan0R29DeOXxiSLMtRzheTaQRVqPmTAfpGd7pJqT7iu"
    "WR0eLrwKNGdm6NdhAVpHfjPLKSPN6mAHac910AUCTr3OhMBHUUx3SEpbLhb93e3bER1hemO4sU"
    "kPFKWyzztMz2IdBNUd3Ob7KoqDMT+cd27UEQe0AqBlXngXtPGZBuQIYJSBnhy6V9iLEsJ1g/"
    "42VHJvjsABRLR8UGT82bEbYWPEhmBECJHBFgDi+nvVgeKYrnvy+vbN+"
    "EfJxaQUNFvu7DEd5q3NNVGctC1Ce4D0UHuClFKqAhcfu7Be5N5IYI0oOpvMPgGA1fmb96DMfGH"
    "uHDBJc4fYINARvKCPuJMR92Ww6PB01z8Il7kH/"
    "C6nrzeIWL0wtcbUQuaP6G1S2fw8UOaDK2QjyM5PsI2+bUCX01wb1DTLzQnlsRXlvC7P3bn/"
    "BBtTsJ/"
    "PnVAjpzANTtDjQfnL2AN20j2NOhRu9fXoY7G1bXgW1zBhBkKqzOeDbbTq2oqcYJV8C5g6hRJvR"
    "Qg9vFt1tkIlQUJaUcnsbbPtCe/hU7vpHSi2/"
    "VPoCy4jvbTKr0VIkKjyCAkDAAxuu8eRFIxOOXVydFxTPkWAQenCY3MyX4eFCs/jPnu/"
    "OXfbBoeDOo8wHpy8lKpvoafRoG6YgXFYKP4GSj63gtwWfhHzl7Skq9JTshAAAAAElFTkSuQmCC"
    "\" title=\"Bing Imagery\"/></a>";

class BingMapsTileProvider final : public QuadtreeRasterOverlayTileProvider {
public:
  BingMapsTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
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
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
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
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
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
            const size_t subdomainIndex =
                (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
            return this->_subdomains[subdomainIndex];
          }
          return key;
        });

    LoadTileImageFromUrlOptions options;
    options.allowEmptyImages = true;
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    std::vector<Credit>& tileCredits = options.credits =
        this->getOwner().getCredits();
    tileCredits.push_back(*this->getCredit());

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);

    // Cesium levels start at 0, Bing levels start at 1
    const unsigned int bingTileLevel = tileID.level + 1;

    for (const CreditAndCoverageAreas& creditAndCoverageAreas : _credits) {
      for (const CoverageArea& coverageArea :
           creditAndCoverageAreas.coverageAreas) {
        if (coverageArea.zoomMin <= bingTileLevel &&
            bingTileLevel <= coverageArea.zoomMax &&
            coverageArea.rectangle.computeIntersection(tileRectangle)
                .has_value()) {
          tileCredits.push_back(creditAndCoverageAreas.credit);
          break;
        }
      }
    }

    return this->loadTileImageFromUrl(url, {}, std::move(options));
  }

private:
  static std::string tileXYToQuadKey(uint32_t level, uint32_t x, uint32_t y) {
    std::string quadkey;
    for (int32_t i = static_cast<int32_t>(level); i >= 0; --i) {
      const uint32_t bitmask = static_cast<uint32_t>(1 << i);
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
    const std::string& name,
    const std::string& url,
    const std::string& key,
    const std::string& mapStyle,
    const std::string& culture,
    const Ellipsoid& ellipsoid,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _url(url),
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
 *       "attribution": "© 2021 Microsoft Corporation",
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
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    bool showCreditsOnScreen) {
  std::vector<CreditAndCoverageAreas> credits;
  const auto attributionsIt = pResource->FindMember("imageryProviders");
  if (attributionsIt != pResource->MemberEnd() &&
      attributionsIt->value.IsArray()) {

    for (const rapidjson::Value& attribution :
         attributionsIt->value.GetArray()) {

      std::vector<CoverageArea> coverageAreas;
      const auto coverageAreasIt = attribution.FindMember("coverageAreas");
      if (coverageAreasIt != attribution.MemberEnd() &&
          coverageAreasIt->value.IsArray()) {

        for (const rapidjson::Value& area : coverageAreasIt->value.GetArray()) {

          const auto bboxIt = area.FindMember("bbox");
          if (bboxIt != area.MemberEnd() && bboxIt->value.IsArray() &&
              bboxIt->value.Size() == 4) {

            const auto zoomMinIt = area.FindMember("zoomMin");
            const auto zoomMaxIt = area.FindMember("zoomMax");
            const auto bboxArrayIt = bboxIt->value.GetArray();
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

      const auto creditString = attribution.FindMember("attribution");
      if (creditString != attribution.MemberEnd() &&
          creditString->value.IsString()) {
        credits.push_back(
            {pCreditSystem->createCredit(
                 creditString->value.GetString(),
                 showCreditsOnScreen),
             coverageAreas});
      }
    }
  }
  return credits;
}

} // namespace

Future<RasterOverlay::CreateTileProviderResult>
BingMapsRasterOverlay::createTileProvider(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    IntrusivePointer<const RasterOverlay> pOwner) const {
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
       culture = this->_culture](
          const std::shared_ptr<IAssetRequest>& pRequest,
          const gsl::span<const std::byte>& data) -> CreateTileProviderResult {
    rapidjson::Document response;
    response.Parse(reinterpret_cast<const char*>(data.data()), data.size());

    if (response.HasParseError()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          fmt::format(
              "Error while parsing Bing Maps imagery metadata, error code "
              "{} at byte offset {}",
              response.GetParseError(),
              response.GetErrorOffset())});
    }

    rapidjson::Value* pError =
        rapidjson::Pointer("/errorDetails/0").Get(response);
    if (pError && pError->IsString()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          fmt::format(
              "Received an error from the Bing Maps imagery metadata service: "
              "{}",
              pError->GetString())});
    }

    rapidjson::Value* pResource =
        rapidjson::Pointer("/resourceSets/0/resources/0").Get(response);
    if (!pResource) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          "Resources were not found in the Bing Maps imagery metadata "
          "response."});
    }

    uint32_t width =
        JsonHelpers::getUint32OrDefault(*pResource, "imageWidth", 256U);
    uint32_t height =
        JsonHelpers::getUint32OrDefault(*pResource, "imageHeight", 256U);
    uint32_t maximumLevel =
        JsonHelpers::getUint32OrDefault(*pResource, "zoomMax", 30U);

    std::vector<std::string> subdomains =
        JsonHelpers::getStrings(*pResource, "imageUrlSubdomains");
    std::string urlTemplate =
        JsonHelpers::getStringOrDefault(*pResource, "imageUrl", std::string());
    if (urlTemplate.empty()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          "Bing Maps tile imageUrl is missing or empty."});
    }

    bool showCredits = pOwner->getOptions().showCreditsOnScreen;
    std::vector<CreditAndCoverageAreas> credits =
        collectCredits(pResource, pCreditSystem, showCredits);
    Credit bingCredit =
        pCreditSystem->createCredit(BING_LOGO_HTML, showCredits);

    return new BingMapsTileProvider(
        pOwner,
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

  auto cacheResultIt = sessionCache.find(metadataUrl);
  if (cacheResultIt != sessionCache.end()) {
    return asyncSystem.createResolvedFuture(
        handleResponse(nullptr, gsl::span<std::byte>(cacheResultIt->second)));
  }

  return pAssetAccessor->get(asyncSystem, metadataUrl)
      .thenInMainThread(
          [metadataUrl,
           handleResponse](std::shared_ptr<IAssetRequest>&& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();

            if (pResponse == nullptr) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  pRequest,
                  "No response received from Bing Maps imagery metadata "
                  "service."});
            }

            CreateTileProviderResult handleResponseResult =
                handleResponse(pRequest, pResponse->data());

            // If the response successfully created a tile provider, cache it.
            if (handleResponseResult) {
              sessionCache[metadataUrl] = std::vector<std::byte>(
                  pResponse->data().begin(),
                  pResponse->data().end());
            }

            return handleResponseResult;
          });
}

} // namespace Cesium3DTilesSelection
