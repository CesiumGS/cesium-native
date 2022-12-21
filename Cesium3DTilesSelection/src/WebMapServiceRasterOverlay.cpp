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
      const IntrusivePointer<const RasterOverlay>& pOwner,
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
      const std::string& layers,
      const std::string& format,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel)
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
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
        _layers(layers),
        _format(format) {}

  virtual ~WebMapServiceTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);

    const std::string urlTemplate =
        this->_url +
        "?request=GetMap&TRANSPARENT=TRUE&version={version}&service="
        "WMS&"
        "format={format}&styles="
        "&width={width}&height={height}&bbox={minx},{miny},{maxx},{maxy}"
        "&layers={layers}&crs=EPSG:4326";

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
        {"layers", this->_layers},
        {"format", this->_format},
        {"width", std::to_string(this->getWidth())},
        {"height", std::to_string(this->getHeight())}};

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });

    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layers;
  std::string _format;
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

static bool validateCapabilities(
    tinyxml2::XMLElement* pRoot,
    const WebMapServiceRasterOverlayOptions& options,
    std::string& error) {
  tinyxml2::XMLElement* pService = pRoot->FirstChildElement("Service");
  if (!pService) {
    error = "Web map service XML document does not have a Service "
            "element. ";
    return false;
  }

  tinyxml2::XMLElement* pServiceName = pService->FirstChildElement("Name");
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
      const int optionalTileWidth = static_cast<int>(options.tileWidth);
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
      const int optionalTileHeight = static_cast<int>(options.tileHeight);
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
      const int numLayers = static_cast<int>(configLayers.size());
      if (numLayers > layerLimit) {
        char buffer[512];
        std::snprintf(
            buffer,
            512,
            "the number of configured layers (%d) exceeds WMS LayerLimit %d",
            numLayers,
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

Future<RasterOverlay::CreateTileProviderResult>
WebMapServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  std::string xmlUrlGetcapabilities =
      CesiumUtility::Uri::substituteTemplateParameters(
          "{baseUrl}?request=GetCapabilities&version={version}&service=WMS",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            } else if (placeholder == "version") {
              return Uri::escape(this->_options.version);
            }
            // Keep other placeholders
            return "{" + placeholder + "}";
          });

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;

  return pAssetAccessor->get(asyncSystem, xmlUrlGetcapabilities, this->_headers)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers =
               this->_headers](const std::shared_ptr<IAssetRequest>& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "No response received from web map service."});
            }

            const gsl::span<const std::byte> data = pResponse->data();

            tinyxml2::XMLDocument doc;
            const tinyxml2::XMLError error = doc.Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());
            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Could not parse web map service XML."});
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map service XML document does not have a root "
                  "element."});
            }

            std::string validationError;
            if (!validateCapabilities(pRoot, options, validationError)) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  validationError});
            }

            const auto projection = CesiumGeospatial::GeographicProjection();

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            const int rootTilesX = 2;
            const int rootTilesY = 1;
            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                coverageRectangle,
                rootTilesX,
                rootTilesY);

            return new WebMapServiceTileProvider(
                pOwner,
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
                options.version,
                options.layers,
                options.format,
                options.tileWidth < 1 ? 1 : uint32_t(options.tileWidth),
                options.tileHeight < 1 ? 1 : uint32_t(options.tileHeight),
                options.minimumLevel < 0 ? 0 : uint32_t(options.minimumLevel),
                options.maximumLevel < 0 ? 0 : uint32_t(options.maximumLevel));
          });
}

} // namespace Cesium3DTilesSelection
