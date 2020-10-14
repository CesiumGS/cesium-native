#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"

namespace Cesium3DTiles {

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider
        ) :
            _pTileProvider(&tileProvider),
            _tileID(0, 0, 0),
            _state(LoadState::Placeholder),
            _pImageRequest(),
            _image(),
            _pRendererResources(nullptr)
        {
        }

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        ) :
            _pTileProvider(&tileProvider),
            _tileID(tileID),
            _state(LoadState::Unloaded),
            _pImageRequest(std::move(pImageRequest)),
            _image(),
            _pRendererResources(nullptr)
        {
            this->_pImageRequest->bind(std::bind(&RasterOverlayTile::requestComplete, this, std::placeholders::_1));
            this->setState(LoadState::Loading);
        }

        RasterOverlayTile::~RasterOverlayTile() {
            TilesetExternals& externals = this->_pTileProvider->getExternals();

            void* pLoadThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? nullptr : this->_pRendererResources;
            void* pMainThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? this->_pRendererResources : nullptr;

            externals.pPrepareRendererResources->freeRaster(*this, pLoadThreadResult, pMainThreadResult);
        }

        void RasterOverlayTile::loadInMainThread() {
            if (this->getState() != RasterOverlayTile::LoadState::Loaded) {
                return;
            }

            this->_pImageRequest.reset();

            // Do the final main thread raster loading
            TilesetExternals& externals = this->_pTileProvider->getExternals();
            this->_pRendererResources = externals.pPrepareRendererResources->prepareRasterInMainThread(*this, this->_pRendererResources);
            this->setState(LoadState::Done);
        }

        void RasterOverlayTile::requestComplete(IAssetRequest* pRequest) {
            IAssetResponse* pResponse = pRequest->response();
            if (pResponse == nullptr) {
                this->setState(LoadState::Failed);
                return;
            }

            if (pResponse->data().size() == 0) {
                this->setState(LoadState::Failed);
                return;
            }
            
            TilesetExternals& externals = this->_pTileProvider->getExternals();
            externals.pTaskProcessor->startTask([pResponse, this]() {
                std::string errors;
                std::string warnings;

                gsl::span<const uint8_t> data = pResponse->data();
                tinygltf::LoadImageData(&this->_image, 0, &errors, &warnings, 0, 0, data.data(), static_cast<int>(data.size()), nullptr);

                // TODO: load failure?
                this->_pRendererResources = this->_pTileProvider->getExternals().pPrepareRendererResources->prepareRasterInLoadThread(*this);

                this->setState(LoadState::Loaded);
            });
        }

        void RasterOverlayTile::setState(LoadState newState) {
            this->_state.store(newState, std::memory_order::memory_order_release);
        }

}
