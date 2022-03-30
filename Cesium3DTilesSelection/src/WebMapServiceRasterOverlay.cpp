#include "Cesium3DTilesSelection/WebMapServiceRasterOverlay.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Uri.h>

#include <cstddef>
#include <sstream>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

class WebMapServiceTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  WebMapServiceTileProvider(
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
      const std::vector<IAssetAccessor::THeader>& headers,
      const std::string& version,
      const std::string& layername,
      const std::string& fileExtension,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel)
      : QuadtreeRasterOverlayTileProvider(
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
        _headers(headers),
        _version(version),
        _layername(layername),
        _fileExtension(fileExtension) {}

  virtual ~WebMapServiceTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
 
    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    //SPDLOG_LOGGER_INFO(getLogger(), "tileID: level>" + std::to_string(tileID.level) + "; x>" + std::to_string(tileID.x) + "; y>" + std::to_string(tileID.y));

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(this->getProjection(), options.rectangle);

    const std::string urlTemplate =
        "{baseUrl}?request=GetMap&TRANSPARENT=TRUE&version={version}&service=wms&"
        "format=image/png&styles="
        "&width={width}&height={height}&bbox={minx},{miny},{maxx},{maxy}"
        "&layers={layername}&crs=EPSG:4326";

    const auto radiansToDegrees = [](double rad) { 
        return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };

    const std::map<std::string, std::string> urlTemplateMap = { 
      {"baseUrl", this->_url},
      {"version", this->_version},
      {"maxx", radiansToDegrees(tileRectangle.getNorth())},
      {"maxy", radiansToDegrees(tileRectangle.getEast())},
      {"minx", radiansToDegrees(tileRectangle.getSouth())},
      {"miny", radiansToDegrees(tileRectangle.getWest())},
      {"layername", this->_layername},
      {"width", std::to_string(this->getWidth())},
      {"height", std::to_string(this->getHeight())}
    };

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [
          &map=urlTemplateMap
        ](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}" : it->second;
        }); 

    //SPDLOG_LOGGER_INFO(getLogger(), url);

    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layername;
  std::string _fileExtension;
};

WebMapServiceRasterOverlay::WebMapServiceRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const WebMapServiceRasterOverlayOptions& wmsOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(wmsOptions) {} 

WebMapServiceRasterOverlay::~WebMapServiceRasterOverlay() {}

static bool validateCapabilities(tinyxml2::XMLElement* pRoot, const WebMapServiceRasterOverlayOptions& options, std::string& error) {
  tinyxml2::XMLElement* pService = pRoot->FirstChildElement("Service");
  if (!pService) {
    error = "Web map service XML document does not have a Service "
            "element. ";
    return false;
  }

  tinyxml2::XMLElement* pServiceName =
      pService->FirstChildElement("Name");
  if (!pServiceName) {
    error = "Invalid web map service XML document (Service > Name is "
            "missing) ";
    return false;
  }

  tinyxml2::XMLElement* pOptionalServiceMaxWidth =
      pService->FirstChildElement("MaxWidth");
  if (pOptionalServiceMaxWidth) {
    const std::string maxWidthText = pOptionalServiceMaxWidth->GetText();
    try {
      const int maxWidth = std::stoi(maxWidthText);
      const int optionalTileWidth = options.tileWidth.value_or(256);
      if (optionalTileWidth > maxWidth) {
        error = fmt::format(
            "configured tile width ({}) exceeds "
            "Service >> "
            "MaxWidth defined in WMS document ({}).",
            optionalTileWidth,
            maxWidth);
        return false;
      }
    } catch (std::invalid_argument&) { 
      error = "Invalid web map service XML document";
      return false;
    }
  }

  tinyxml2::XMLElement* pOptionalServiceMaxHeight =
      pService->FirstChildElement("MaxHeight");
  if (pOptionalServiceMaxHeight) {
    const std::string maxHeightText = pOptionalServiceMaxHeight->GetText();
    try {
      const int maxHeight = std::stoi(maxHeightText);
      const int optionalTileHeight = options.tileHeight.value_or(256);
      if (optionalTileHeight > maxHeight) {
        error = fmt::format(
            "configured tile height ({}) exceeds "
            "Service >> "
            "MaxHeight defined in WMS document ({}).",
            optionalTileHeight,
            maxHeight);
        return false;
      }
    } catch (std::invalid_argument&) {
      error = "Invalid web map service XML document";
      return false;
    }
  } 

  std::vector<std::string> configLayers{};
  std::stringstream sstream(options.layers);
  std::string layer;
  const char delimiter = ',';
  while (std::getline(sstream, layer, delimiter)) { 
    if (layer.size() > 0) { 
      configLayers.push_back(layer);
    }
  } 
 
  tinyxml2::XMLElement* pOptionalServiceLayerLimit =
      pService->FirstChildElement("LayerLimit");
  if (pOptionalServiceLayerLimit) { 
    try {
      const int layerLimit = std::stoi(pOptionalServiceLayerLimit->GetText()); 
      if (configLayers.size() > layerLimit) { 
        char buffer[512];
        std::sprintf(
            buffer,
            "the number of configured layers (%d) exceeds WMS LayerLimit %d",
            static_cast<int>(configLayers.size()),
            layerLimit);
        error = buffer;
        return false;
      }
    } catch (std::invalid_argument const&) { 
      error = "Invalid web map service XML document";
      return false;
    }
  } 

  return true;
}

Future<std::unique_ptr<RasterOverlayTileProvider>>
WebMapServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner)
{

  std::string xmlUrlGetcapabilities = CesiumUtility::Uri::substituteTemplateParameters(
      "{baseUrl}?request=GetCapabilities&version={version}&service=WMS",
      [this](const std::string& placeholder) {
        if (placeholder == "baseUrl") {
          return this->_baseUrl;
        } else if (placeholder == "version") {
          return this->_options.version.value();
        }
        // Keep other placeholders
        return "{" + placeholder + "}";
      });

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;

  auto reportError = [this, asyncSystem, pLogger](
                         const std::shared_ptr<IAssetRequest>& pRequest,
                         const std::string& message) {
    this->reportError(
        asyncSystem,
        pLogger,
        RasterOverlayLoadFailureDetails{
            this,
            RasterOverlayLoadType::TileProvider,
            pRequest,
            message});
  };

  return pAssetAccessor->get(asyncSystem, xmlUrlGetcapabilities, this->_headers)
      .thenInWorkerThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers = this->_headers,
           reportError](const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlayTileProvider> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              reportError(
                  pRequest,
                  "No response received from web map service.");
              return nullptr;
            }

            const gsl::span<const std::byte> data = pResponse->data();

            tinyxml2::XMLDocument doc;
            const tinyxml2::XMLError error = doc.Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());
            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              reportError(pRequest, "Could not parse web map service XML.");
              return nullptr;
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot) {
              reportError(
                  pRequest,
                  "Web map service XML document does not have a root "
                  "element.");
              return nullptr;
            }

            std::string validationError;
            if (!validateCapabilities(pRoot, options, validationError)) { 
              reportError(pRequest, validationError);
              return nullptr;
            }

            const auto projection = CesiumGeospatial::GeographicProjection();

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            const int rootTilesX = 2;
            const int rootTilesY = 1;
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(coverageRectangle, rootTilesX, rootTilesY);

            return std::make_unique<WebMapServiceTileProvider>(
                *pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pPrepareRendererResources,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle, 
                url,
                headers,
                options.version.value(),
                options.layers,
                options.fileExtension.has_value() ? "." + options.fileExtension.value() : "",
                options.tileWidth.value(),
                options.tileHeight.value(),
                options.minimumLevel.value(),
                options.maximumLevel.value()
              );
          });
}

} // namespace Cesium3DTilesSelection
