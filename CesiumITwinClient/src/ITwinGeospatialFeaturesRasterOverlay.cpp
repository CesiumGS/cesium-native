#include "CesiumAsync/Future.h"
#include "CesiumRasterOverlays/VectorDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorStyle.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumITwinClient/ITwinGeospatialFeaturesRasterOverlay.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorNode.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <glm/common.hpp>
#include <glm/ext/vector_int2.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/logger.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
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
    const CesiumVectorData::VectorStyle& style,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    uint32_t mipLevels,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _iTwinId(iTwinId),
      _collectionId(collectionId),
      _pConnection(pConnection),
      _style(style),
      _ellipsoid(ellipsoid),
      _projection(projection),
      _mipLevels(mipLevels) {}

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
      .thenInWorkerThread([asyncSystem, pConnection = this->_pConnection](
                              Result<PagedList<VectorNode>>&& result) mutable {
        if (!result.value) {
          return asyncSystem.createResolvedFuture(
              Result<std::vector<VectorNode>>(result.errors));
        }

        return result.value->allAfter(asyncSystem, pConnection);
      })
      .thenInWorkerThread(
          [asyncSystem,
           name = this->getName(),
           style = this->_style,
           projection = this->_projection,
           ellipsoid = this->_ellipsoid,
           mipLevels = this->_mipLevels,
           options = this->getOptions(),
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           pLogger,
           pOwner](Result<std::vector<VectorNode>>&& result)
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

            IntrusivePointer<VectorDocument> pDocument;
            pDocument.emplace(VectorNode{}, std::vector<VectorDocumentAttribution>{});
            pDocument->getRootNode().children = std::move(*result.value);
            VectorDocumentRasterOverlay vectorOverlay(
                name,
                pDocument,
                style,
                projection,
                ellipsoid,
                mipLevels,
                options);

            return vectorOverlay.createTileProvider(
                asyncSystem,
                pAssetAccessor,
                pCreditSystem,
                pPrepareRendererResources,
                pLogger,
                pOwner);
          });
}

} // namespace CesiumITwinClient
