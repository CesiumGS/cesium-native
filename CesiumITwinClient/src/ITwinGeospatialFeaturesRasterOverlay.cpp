#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumITwinClient/ITwinGeospatialFeaturesRasterOverlay.h>
#include <CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/VectorRasterizer.h>
#include <CesiumVectorData/VectorStyle.h>

#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/logger.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumITwinClient {

ITwinGeospatialFeaturesRasterOverlay::ITwinGeospatialFeaturesRasterOverlay(
    const std::string& name,
    const std::string& iTwinId,
    const std::string& collectionId,
    const CesiumUtility::IntrusivePointer<Connection>& pConnection,
    const CesiumRasterOverlays::GeoJsonDocumentRasterOverlayOptions&
        geoJsonOptions,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _iTwinId(iTwinId),
      _collectionId(collectionId),
      _pConnection(pConnection),
      _options(geoJsonOptions) {}

ITwinGeospatialFeaturesRasterOverlay::~ITwinGeospatialFeaturesRasterOverlay() =
    default;

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
ITwinGeospatialFeaturesRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  pOwner = pOwner ? pOwner : this;

  return this->_pConnection
      ->geospatialFeatures(this->_iTwinId, this->_collectionId)
      .thenInWorkerThread(
          [asyncSystem, pConnection = this->_pConnection](
              Result<PagedList<GeoJsonFeature>>&& result) mutable {
            if (!result.value) {
              return asyncSystem.createResolvedFuture(
                  Result<std::vector<GeoJsonFeature>>(result.errors));
            }

            return result.value->allAfter(asyncSystem, pConnection);
          })
      .thenInWorkerThread(
          [asyncSystem,
           name = this->getName(),
           vectorOptions = this->_options,
           options = this->getOptions(),
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           pLogger,
           pOwner](Result<std::vector<GeoJsonFeature>>&& result)
              -> CesiumAsync::Future<RasterOverlay::CreateTileProviderResult> {
            if (!result.value) {
              return asyncSystem.createResolvedFuture<
                  RasterOverlay::CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      RasterOverlayLoadType::CesiumIon,
                      nullptr,
                      fmt::format(
                          "Errors while loading geospatial features: {}",
                          CesiumUtility::joinToString(
                              result.errors.errors,
                              ", "))}));
            }

            std::vector<GeoJsonObject> objects;
            objects.reserve(result.value->size());

            for (GeoJsonFeature& feature : *result.value) {
              objects.emplace_back(std::move(feature));
            }

            std::shared_ptr<GeoJsonDocument> pDocument =
                std::make_shared<GeoJsonDocument>(
                    GeoJsonObject{GeoJsonFeatureCollection{
                        std::move(objects),
                        std::nullopt,
                        {},
                        std::nullopt}},
                    std::vector<VectorDocumentAttribution>{});
            GeoJsonDocumentRasterOverlay geoJsonOverlay(
                name,
                std::move(pDocument),
                vectorOptions,
                options);

            return geoJsonOverlay.createTileProvider(
                asyncSystem,
                pAssetAccessor,
                pCreditSystem,
                pPrepareRendererResources,
                pLogger,
                pOwner);
          });
}

} // namespace CesiumITwinClient